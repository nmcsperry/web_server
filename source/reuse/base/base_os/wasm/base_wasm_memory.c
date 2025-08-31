#include "../../base_include.h"

extern u8 memory;

#define WasmMemoryDivisions 8
#define WasmMemoryTotalSize 67108864

void * OSReserveMemory(i32 Bytes)
{
	static u8 Counter = 0;

	i32 DivisionSize = WasmMemoryTotalSize / WasmMemoryDivisions;
	if (Bytes > DivisionSize)
	{
		return 0;
	}
	else if (Counter >= WasmMemoryDivisions)
	{
		return 0;
	}
	else
	{
		// todo: grow the memory region with each allocation instead of having it be all filled instantly
		// (there is like __builtin_grow or something)

		u8 * WasmMemoryPtr = &memory;
		void * Result = WasmMemoryPtr + DivisionSize * Counter;
		Counter++;

		return Result;
	}
}

#undef WasmMemoryDivisions
#undef WasmMemoryTotalSize

void OSCommitMemory(void * Memory, i32 Bytes) 
{
	return;
}

void OSReleaseMemory(void * Memory, i32 Bytes)
{
	return;
}

void OSDecommitMemory(void * Memory, i32 Bytes)
{
	return;
}

void OSCopyMemory(void * Dest, void * Source, u32 Size)
{
	__builtin_memcpy(Dest, Source, Size);
}

void OSZeroMemory(void * Dest, u32 Size)
{
	__builtin_memset(Dest, 0, Size);
}

void OSMoveMemory(void * Source, u32 Size, void * Dest)
{
	__builtin_memcpy(Dest, Source, Size);
}