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
		Result.Pages[BufferIndex].PageBufferIndex = BufferIndex;
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

	i32 NextScanIndex = BufferPool->ScanIndex;

	for (i32 BufferOffset = 0; BufferOffset < BUFFER_POOL_SIZE; BufferOffset++)
	{
		i32 BufferIndex = (BufferOffset + BufferPool->ScanIndex) % BUFFER_POOL_SIZE;
		page_slot * Page = &BufferPool->Pages[BufferIndex];

		if (Page->Info.Valid && Page->Header.PageId == PageId)
		{
			Candidate = BufferIndex;
			ReplacementType = BPReplacement_AlreadyLoaded;

			break;
		}

		if (ReplacementType < BPReplacement_Clock)
		{
			NextScanIndex++;
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

	BufferPool->ScanIndex = NextScanIndex % BUFFER_POOL_SIZE;

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

void PageMarkAccess(page_slot * Page)
{
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
	if (Page->Info.Valid)
	{
		PageSyncHeaderToData(Page);

		str8 Data = (str8) { .Data = Page->Data, .Count = DB_PAGE_SIZE };
		FileOutputSegment(File, Data, Page->Header.PageId * DB_PAGE_SIZE);

		FileClose(File);
	}
}

void BufferPoolLoad(buffer_pool * BufferPool, i32 BufferPoolIndex, u32 PageId)
{
	BufferPoolEvict(BufferPool, BufferPoolIndex);

	file_handle File = FileOpenInput("test.database");

	page_slot * Page = &BufferPool->Pages[BufferPoolIndex];
	FileInputSegmentToPtr(File, Page->Data, PageId * DB_PAGE_SIZE, DB_PAGE_SIZE);

	FileClose(File);
}

void BufferPoolFlush(buffer_pool * BufferPool, bool32 Evict)
{
	for (int BufferIndex = 0; BufferIndex < BUFFER_POOL_SIZE; BufferIndex++)
	{
		if (Evict)
		{
			BufferPoolEvict(BufferPool, BufferIndex);
		}
		else
		{
			BufferPoolSave(BufferPool, BufferIndex);
		}
	}
}

db_context DBContextNew(db_context * Context)
{
	db_context NewContext = { 0 };
	NewContext.BufferPool = Context->BufferPool;
	return NewContext;
}

page_slot * DBContextPin(db_context * Context, page_slot * Page)
{
	Page->Info.Pins++;
	Context->Pins[Page->PageBufferIndex]++;
	return Page;
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

page_slot * PageLoad(db_context * Context, u32 PageId)
{
	bp_index BufferPoolIndex = BufferPoolFindAvailableIndex(Context->BufferPool, PageId);
	page_slot * Page = &Context->BufferPool->Pages[BufferPoolIndex.Index];

	if (BufferPoolIndex.ReplacementType != BPReplacement_AlreadyLoaded)
	{
		BufferPoolLoad(Context->BufferPool, BufferPoolIndex.Index, PageId);
		PageSyncHeaderFromData(Page);
		PageInit(Page);
	}

	return DBContextPin(Context, Page);
}

page_slot * PageNew(db_context * Context, page_header Header)
{
	Header.PageId = Context->BufferPool->NextFreePageId;
	Context->BufferPool->NextFreePageId++; // todo: when we delete pages, mark them as unused, reuse them, etc.

	bp_index BufferPoolIndex = BufferPoolFindAvailableIndex(Context->BufferPool, -1);
	BufferPoolEvict(Context->BufferPool, BufferPoolIndex.Index);
	page_slot * Page = &Context->BufferPool->Pages[BufferPoolIndex.Index];

	Page->Header = Header;
	PageSyncHeaderToData(Page);
	PageInit(Page);

	return DBContextPin(Context, Page);
}

db_location PageDirAppend(db_context * Context, page_slot * DirectoryPage, void * Data, u32 SourceSize)
{
	db_context TempContext = DBContextNew(Context);

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
		page_slot * LastPage = PageLoad(&TempContext, LastPageId);

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
		NewPageHeader.DirectoryPageId = DirectoryPage->Header.PageId;
		NewPageHeader.PreviousPageId = LastPageId;
		NewPageHeader.NextPageId = 0;
		NewPageHeader.ElementSize2 = 0;

		page_slot * NewPage = PageNew(&TempContext, NewPageHeader);

		if (LastPage)
		{
			LastPage->Header.NextPageId = NewPage->Header.PageId;
			LastPage->Info.Dirty = true;
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
	Page->Header.ElementCount++;
	Page->Info.Dirty = true;

	PageMarkAccess(Page);

	return NewIndex;
}

blob PageGet(page_slot * Page, u32 Index)
{
	void * Pointer = (u8 *) Page->Info.Body + Page->Header.ElementSize * (Index - 1);
	PageMarkAccess(Page);
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
	Page->Info.Valid = true;

	PageMarkAccess(Page);
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