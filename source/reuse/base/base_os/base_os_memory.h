#ifndef base_os_memory_h
#define base_os_memory_h

#include "../base_types.h"

void * OSReserveMemory(i32 Bytes);

void OSCommitMemory(void * Memory, i32 Bytes);

void OSReleaseMemory(void * Memory, i32 Bytes);

void OSDecommitMemory(void * Memory, i32 Bytes);

void OSCopyMemory(void * Dest, void * Source, u32 Size);

void OSZeroMemory(void * Dest, u32 Size);

void OSMoveMemory(void * Source, u32 Size, void * Dest);

#endif