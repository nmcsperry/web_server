#ifndef database_h
#define database_h

#include "../../base/base_include.h"

enum {
	PageType_Directory = 1,
	PageType_Elements = 2
};

#pragma pack(push, 1)
typedef struct page_header_packed
{
	u32 PageId;
	u32 PageType;
	u32 ElementSize;
	u32 ElementCount;
	u32 DirectoryPageId;
	u32 PreviousPageId;
	u32 NextPageId;
	u32 ElementSize2;
} page_header_packed;
#pragma pack(pop)

typedef struct page_header
{
	u32 PageId;
	u32 PageType;
	u32 ElementSize;
	u32 ElementCount;
	u32 DirectoryPageId;
	u32 PreviousPageId;
	u32 NextPageId;
	u32 ElementSize2;
} page_header;

typedef struct page_info
{
	void * Header;
	void * Body;

	u64 LastAccessUSec;

	bool32 Dirty;
	bool32 Used;
	bool32 Valid;
	u8 Pins;
} page_info;

typedef struct page_slot
{
	void * Data;
	i32 PageBufferIndex;

	page_info Info;
	page_header Header;
} page_slot;

typedef struct page_slot_temp
{
	page_slot * Page;
	u32 PageId;
} page_slot_temp;

typedef struct page_ref
{
	u32 PageId;
	i32 PageBufferIndex;
} page_ref;

typedef struct db_location
{
	u32 PageId;
	u32 Index;
	bool32 Error;
} db_location;

page_ref PageRefFromId(u32 PageId)
{
	return (page_ref) { .PageId = PageId, .PageBufferIndex = -1 };
}

page_ref PageRefFromPage(page_slot * Page)
{
	return (page_ref) { .PageId = Page->Header.PageId, .PageBufferIndex = Page->PageBufferIndex };
}

page_slot_temp PageTempFromPage(page_slot * Page)
{
	return (page_slot_temp) { .Page = Page, .PageId = Page->Header.PageId };
}

page_ref PageRefFromTemp(page_slot_temp Page)
{
	if (Page.Page->Header.PageId == Page.PageId)
	{
		return PageRefFromPage(Page.Page);
	}
	else
	{
		return PageRefFromId(Page.PageId);
	}
}

#define DB_PAGE_SIZE Kilobytes(16)
#define BUFFER_POOL_SIZE 64

typedef struct buffer_pool
{
	void * Data;
	page_slot Pages[BUFFER_POOL_SIZE];

	u32 NextFreePageId;

	i32 ScanIndex;
} buffer_pool;

typedef struct db_context
{
	buffer_pool * BufferPool;
	u8 Pins[BUFFER_POOL_SIZE];
} db_context;

typedef enum bp_replacement {
	BPReplacement_Fallback,
	BPReplacement_LRU,
	BPReplacement_Clock,
	BPReplacement_Invalid,
	BPReplacement_AlreadyLoaded,
} bp_replacement;

typedef struct bp_index {
	i32 Index;
	bp_replacement ReplacementType;
} bp_index;

db_location DBLocationError()
{
	return (db_location) { .Index = 0, .PageId = 0, .Error = true };
}

buffer_pool BufferPoolInit();
bp_index BufferPoolFindAvailableIndex(buffer_pool * BufferPool, u32 PageId);
void BufferPoolEvict(buffer_pool * BufferPool, i32 BufferPoolIndex);
void BufferPoolMarkAccess(buffer_pool * BufferPool, i32 BufferPoolIndex);
void BufferPoolSave(buffer_pool * BufferPool, i32 BufferPoolIndex);
void BufferPoolLoad(buffer_pool * BufferPool, i32 BufferPoolIndex, u32 PageId);

db_context DBContextNew(db_context * Context);
page_slot * DBContextPin(db_context * Context, page_slot_temp Page);
void DBContextUnpin(db_context * Context, page_slot * Page);
void DBContextClose(db_context * Context);

page_slot_temp PageLoadTemp(db_context * Context, u32 PageId);
page_slot * PageLoadPin(db_context * Context, u32 PageId);
page_ref PageLoad(db_context * Context, u32 PageId);

page_slot_temp PageNewTemp(db_context * Context, page_header Header);
page_slot * PageNewPin(db_context * Context, page_header Header);
page_ref PageNew(db_context * Context, page_header Header);

db_location PageDirAppend(db_context * Context, u32 DirectoryPageId, void * Data, u32 SourceSize);

u32 PageSave(page_slot * Page);
u32 PageSaveTemp(page_slot_temp Page);

u32 PageAppend(page_slot * Page, void * Data, u32 SourceSize);
u32 PageAppendTemp(page_slot_temp Page, void * Data, u32 SourceSize);

blob PageGet(page_slot * Page, u32 Index);
blob PageGetTemp(page_slot_temp Page, u32 Index);

u32 PageSpaceLeft(page_slot * Page);
u32 PageSpaceLeftTemp(page_slot_temp Page);

bool32 PageInit(page_slot * Page);
bool32 PageInitTemp(page_slot_temp Page);

bool32 PageSyncHeaderFromData(page_slot * Page);
bool32 PageSyncHeaderFromDataTemp(page_slot_temp Page);

bool32 PageSyncHeaderToData(page_slot * Page);
bool32 PageSyncHeaderToDataTemp(page_slot_temp Page);

#endif