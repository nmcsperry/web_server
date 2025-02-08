#include "../../base/base_include.h"
#include "web_server_include.h"
#include "../../io/io_include.h"

buffer_pool BufferPoolInit()
{
	buffer_pool Result = { 0 };

	Result.Data = OSAllocateMemory(BUFFER_POOL_SIZE * DB_PAGE_SIZE);

	for (int BufferIndex = 0; BufferIndex < BUFFER_POOL_SIZE; BufferIndex++)
	{
		Result.Pages[BufferIndex].Data = (u8 *) Result.Data + DB_PAGE_SIZE * BufferIndex;
	}

	file_handle File = FileOpenInput("test.database");
	u32 FileSize = FileGetSize(File);
	FileClose(File);

	Result.NextFreePageId = FileSize / DB_PAGE_SIZE + 1;

	return Result;
}

bp_index BufferPoolFindAvailableIndex(buffer_pool * BufferPool, u32 PageId)
{
	// return an index where we can store a page id, in order of preference:
	// - the buffer slot already containing that page id (already loaded)
	// - a buffer slot not loaded with anything yet (invalid)
	// - the next buffer slot that has not been used since we tried this before (clock replacement)
	// - the buffer slot that has been unused for the longest amount of time (LRU)
	// - the first buffer slot (fallback, should never happen)

	int Candidate = 0;
	bp_replacement ReplacementType = BPReplacement_Fallback;

	u64 LRUPageId = -1;
	u64 LRULastAccessUSec = U64Max;

	for (int BufferOffset = 0; BufferOffset < BUFFER_POOL_SIZE; BufferOffset++)
	{
		int BufferIndex = (BufferOffset + BufferPool->ScanIndex) % BUFFER_POOL_SIZE;
		page_slot * Page = &BufferPool->Pages[BufferIndex];

		if (Page->Info.Valid && Page->Header.PageId == PageId)
		{
			return (bp_index) { BufferIndex, BPReplacement_AlreadyLoaded };
		}

		if (ReplacementType < BPReplacement_Clock)
		{
			BufferPool->ScanIndex++;
			if (!Page->Info.Used)
			{
				Candidate = BufferIndex;
				ReplacementType = BPReplacement_Clock;
			}
			else
			{
				Page->Info.Used = false;
			}
		}

		if (ReplacementType < BPReplacement_Invalid && !Page->Info.Valid)
		{
			Candidate = BufferIndex;
			ReplacementType = BPReplacement_Invalid;
		}

		if (Page->Info.Valid && Page->Info.LastAccessUSec < LRULastAccessUSec)
		{
			LRUPageId = Page->Header.PageId;
			LRULastAccessUSec = Page->Info.LastAccessUSec;
		}
	}

	if (ReplacementType == BPReplacement_Fallback && LRUPageId != -1)
	{
		Candidate = LRUPageId;
		ReplacementType = BPReplacement_LRU;
	}

	return (bp_index) { Candidate, ReplacementType };
}

void BufferPoolEvict(buffer_pool * BufferPool, i32 BufferPoolIndex)
{
	page_slot * Page = &BufferPool->Pages[BufferPoolIndex];

	if (Page->Info.Valid && Page->Info.Dirty)
	{
		BufferPoolSave(BufferPool, BufferPoolIndex);
	}

	OSZeroMemory(Page->Data, DB_PAGE_SIZE);
	Page->Header = (page_header) { 0 };
	Page->Info = (page_info) { 0 };

	Page->Info.Valid = false;
}

void BufferPoolMarkAccess(buffer_pool * BufferPool, i32 BufferPoolIndex)
{
	page_slot * Page = &BufferPool->Pages[BufferPoolIndex];

	if (Page->Info.Valid)
	{
		Page->Info.LastAccessUSec = UnixTimeUSec();
		Page->Info.Used = true;
	}
}

void BufferPoolSave(buffer_pool * BufferPool, i32 BufferPoolIndex)
{
	file_handle File = FileOpenOutput("test.database", false);

	page_slot * Page = &BufferPool->Pages[BufferPoolIndex];
	PageSyncHeaderToData(Page);

	str8 Data = (str8) { .Data = Page->Data, .Count = DB_PAGE_SIZE };
	FileOutputSegment(File, Data, Page->Header.PageId * DB_PAGE_SIZE);

	FileClose(File);
}

void BufferPoolLoad(buffer_pool * BufferPool, i32 BufferPoolIndex, u32 PageId)
{
	BufferPoolEvict(BufferPool, BufferPoolIndex);

	file_handle File = FileOpenInput("test.database");

	page_slot * Page = &BufferPool->Pages[BufferPoolIndex];
	FileInputSegmentToPtr(File, Page->Data, Page->Header.PageId * DB_PAGE_SIZE, DB_PAGE_SIZE);

	FileClose(File);
}

db_context DBContextNew(db_context * Context)
{
	db_context NewContext = { 0 };
	NewContext.BufferPool = Context->BufferPool;
	return NewContext;
}

page_slot * DBContextPin(db_context * Context, page_slot_temp Page)
{
	if (Page.Page->Header.PageId != Page.PageId)
	{
		return 0;
	}

	Page.Page->Info.Pins++;
	Context->Pins[Page.Page->PageBufferIndex]++;

	return Page.Page;
}

void DBContextUnpin(db_context * Context, page_slot * Page)
{
	Page->Info.Pins--;
	Context->Pins[Page->PageBufferIndex]--;
}

void DBContextClose(db_context * Context)
{
	for (i32 BufferPoolIndex = 0; BufferPoolIndex < BUFFER_POOL_SIZE; BufferPoolIndex++)
	{
		Context->BufferPool->Pages[BufferPoolIndex].Info.Pins -= Context->Pins[BufferPoolIndex];
		Context->Pins[BufferPoolIndex] = 0;
	}
}

page_slot_temp PageLoadTemp(db_context * Context, u32 PageId)
{
	bp_index BufferPoolIndex = BufferPoolFindAvailableIndex(Context->BufferPool, PageId);
	page_slot * Page = &Context->BufferPool->Pages[BufferPoolIndex.Index];

	if (BufferPoolIndex.ReplacementType != BPReplacement_AlreadyLoaded)
	{
		BufferPoolLoad(Context->BufferPool, BufferPoolIndex.Index, PageId);
		PageSyncHeaderFromData(Page);
		PageInit(Page);
	}

	return PageTempFromPage(Page);
}

page_slot_temp PageNewTemp(db_context * Context, page_header Header)
{
	Header.PageId = Context->BufferPool->NextFreePageId;
	Context->BufferPool->NextFreePageId++; // todo: when we delete pages, mark them as unused, reuse them, etc.

	bp_index BufferPoolIndex = BufferPoolFindAvailableIndex(Context->BufferPool, -1);
	BufferPoolEvict(Context->BufferPool, BufferPoolIndex.Index);
	page_slot * Page = &Context->BufferPool->Pages[BufferPoolIndex.Index];

	Page->Header = Header;
	PageSyncHeaderToData(Page);
	PageInit(Page);

	return PageTempFromPage(Page);
}

db_location PageDirAppend(db_context * Context, u32 DirectoryPageId, void * Data, u32 SourceSize)
{
	db_context TempContext = DBContextNew(Context);

	page_slot * DirectoryPage = PageLoadPin(&TempContext, DirectoryPageId);
	if (DirectoryPage->Header.PageType != PageType_Directory)
	{
		return DBLocationError();
	}

	page_slot * Page = 0;
	u32 LastPageId = 0;
	page_slot * LastPage = 0;

	if (DirectoryPage->Header.ElementCount != 0)
	{
		LastPageId = *((u32 *) PageGet(DirectoryPage, DirectoryPage->Header.ElementCount).Data);
		page_slot * LastPage = PageLoadPin(&TempContext, LastPageId);

		if (PageSpaceLeft(LastPage) > LastPage->Header.ElementSize)
		{
			Page = LastPage;
		}
	}

	if (Page == 0)
	{
		page_header NewPageHeader = { 0 };
		NewPageHeader.PageId = 0;
		NewPageHeader.PageType = PageType_Elements;
		NewPageHeader.ElementSize = DirectoryPage->Header.ElementSize2;
		NewPageHeader.ElementCount = 0;
		NewPageHeader.DirectoryPageId = DirectoryPageId;
		NewPageHeader.PreviousPageId = LastPageId;
		NewPageHeader.NextPageId = 0;
		NewPageHeader.ElementSize2 = 0;

		page_slot * NewPage = PageNewPin(&TempContext, NewPageHeader);

		if (LastPage)
		{
			LastPage->Header.NextPageId = NewPage->Header.PageId;
		}
		PageAppend(DirectoryPage, &NewPage->Header.PageId, sizeof(u32));

		Page = NewPage;
	}

	u32 Index = PageAppend(Page, Data, SourceSize);

	DBContextClose(&TempContext);

	return (db_location) { .Index = Index, .PageId = Page->Header.PageId };
}

u32 PageAppend(page_slot * Page, void * Data, u32 SourceSize)
{
	u32 LastIndex = Page->Header.ElementCount;
	u32 NewIndex = LastIndex + 1;
	void * Pointer = (u8 *) Page->Info.Body + Page->Header.ElementSize * (NewIndex - 1);
	OSCopyMemory(Pointer, Data, SourceSize);
	return NewIndex;
}

blob PageGet(page_slot * Page, u32 Index)
{
	void * Pointer = (u8 *) Page->Info.Body + Page->Header.ElementSize * (Index - 1);
	return (blob) { .Data = Pointer, .Count = Page->Header.ElementSize };
}

u32 PageSpaceLeft(page_slot * Page)
{
	u32 TotalSize = DB_PAGE_SIZE - sizeof(page_header_packed);
	u32 UsedSize = Page->Header.ElementSize * Page->Header.ElementCount;
	return TotalSize - UsedSize;
}

bool32 PageInit(page_slot * Page)
{
	Page->Info.Body = (u8 *) Page->Data + sizeof(page_header_packed);
	Page->Info.Header = Page->Data;
	Page->Info.LastAccessUSec = UnixTimeUSec();
	Page->Info.Valid = true;
}

bool32 PageSyncHeaderFromData(page_slot * Page)
{
	page_header_packed * Source = Page->Data;
	page_header * Destination = &Page->Header;

	Destination->PageId = Source->PageId;
	Destination->PageType = Source->PageType;
	Destination->ElementSize = Source->ElementSize;
	Destination->ElementCount = Source->ElementCount;
	Destination->DirectoryPageId = Source->DirectoryPageId;
	Destination->PreviousPageId = Source->PreviousPageId;
	Destination->NextPageId = Source->NextPageId;
	Destination->ElementSize2 = Source->ElementSize2;
}

bool32 PageSyncHeaderToData(page_slot * Page)
{
	page_header * Source = &Page->Header;
	page_header_packed * Destination = Page->Data;

	Destination->PageId = Source->PageId;
	Destination->PageType = Source->PageType;
	Destination->ElementSize = Source->ElementSize;
	Destination->ElementCount = Source->ElementCount;
	Destination->DirectoryPageId = Source->DirectoryPageId;
	Destination->PreviousPageId = Source->PreviousPageId;
	Destination->NextPageId = Source->NextPageId;
	Destination->ElementSize2 = Source->ElementSize2;
}

#define DEFINE_PIN_FUNCTION(BaseName, ...) { page_slot_temp Page = BaseName##Temp(__VA_ARGS__); return DBContextPin(Context, Page); }
#define DEFINE_REF_FUNCTION(BaseName, ...) { page_slot_temp Page = BaseName##Temp(__VA_ARGS__); return PageRefFromTemp(Page); }
#define DEFINE_TEMP_FUNCTION(BaseName, ...) { page_slot * PagePointer = Page.Page; return BaseName(PagePointer, __VA_ARGS__); }

page_slot * PageLoadPin(db_context * Context, u32 PageId) DEFINE_PIN_FUNCTION(PageLoad, Context, PageId)
page_ref PageLoad(db_context * Context, u32 PageId) DEFINE_REF_FUNCTION(PageLoad, Context, PageId)

page_slot * PageNewPin(db_context * Context, page_header Header) DEFINE_PIN_FUNCTION(PageNew, Context, Header)
page_ref PageNew(db_context * Context, page_header Header) DEFINE_REF_FUNCTION(PageNew, Context, Header)

u32 PageAppendTemp(page_slot_temp Page, void * Data, u32 SourceSize) DEFINE_TEMP_FUNCTION(PageAppend, Data, SourceSize)

blob PageGetTemp(page_slot_temp Page, u32 Index) DEFINE_TEMP_FUNCTION(PageGet, Index)

u32 PageSpaceLeftTemp(page_slot_temp Page) DEFINE_TEMP_FUNCTION(PageSpaceLeft)

bool32 PageInitTemp(page_slot_temp Page) DEFINE_TEMP_FUNCTION(PageInit)

bool32 PageSyncHeaderFromDataTemp(page_slot_temp Page) DEFINE_TEMP_FUNCTION(PageSyncHeaderFromData)

bool32 PageSyncHeaderToDataTemp(page_slot_temp Page) DEFINE_TEMP_FUNCTION(PageSyncHeaderToData)

#undef DEFINE_PIN_FUNCTION
#undef DEFINE_REF_FUNCTION
#undef DEFINE_TEMP_FUNCTION