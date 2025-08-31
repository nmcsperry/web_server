#include "../../base_include.h"

void * OSReserveMemory(i32 Bytes)
{
	return VirtualAlloc(0, Bytes, MEM_RESERVE, PAGE_READWRITE);
}

void OSCommitMemory(void * Memory, i32 Bytes) 
{
	void * Result = VirtualAlloc(Memory, Bytes, MEM_COMMIT, PAGE_READWRITE);
}

void OSReleaseMemory(void * Memory, i32 Bytes)
{
	VirtualFree(Memory, 0, MEM_RELEASE);
}

void OSDecommitMemory(void * Memory, i32 Bytes)
{
	VirtualFree(Memory, Bytes, MEM_DECOMMIT);
}

void OSCopyMemory(void * Dest, void * Source, u32 Size)
{
	CopyMemory(Dest, Source, Size);
}

void OSZeroMemory(void * Dest, u32 Size)
{
	ZeroMemory(Dest, Size);
}

void OSMoveMemory(void * Source, u32 Size, void * Dest)
{
	MoveMemory(Dest, Source, Size);
}