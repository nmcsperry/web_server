#ifndef base_memory_h
#define base_memory_h

#ifdef __cplusplus
extern "C" {
#endif

void * OSAllocateMemory(u32 Bytes);
void OSFreeMemory(void * Memory, u32 Bytes);

// arena

#define DEFAULT_ARENA_COMMIT_SIZE 65536 // this must be a multiple of page size (also it must be a power of two)
#define DEFAULT_ARENA_SIZE Kilobytes(64)
#define MINIMUM_ARENA_SIZE Kilobytes(4)

enum {
	MemoryArenaFlag_ProgressiveCommit = 1,
	MemoryArenaFlag_ProgressiveBlockReserve = 2
};

typedef struct memory_arena_block memory_arena_block;

typedef struct memory_arena {
	memory_arena_block * First;
	memory_arena_block * Last;

	u32 CommitSize;
	u32 BlockSize;

	u32 MaxCount;
	u32 Count;

	u8 Flags;
} memory_arena;

struct memory_arena_block {
	memory_arena_block * Next;
	memory_arena_block * Previous;
	memory_arena * Arena;

	u32 Offset;
	u32 MaxCount;
	u32 Count;
	u32 CommitCount;
};

#define ARENA_BASE_SIZE (AlignPow2(sizeof(memory_arena_block), alignof(memory_arena)) + sizeof(memory_arena))

memory_arena * ArenaCreate(u32 Bytes);
memory_arena * ArenaCreateAdv(u32 Bytes, u32 CommitSize, u32 BlockSize);

void ArenaReset(memory_arena * Arena);
void * ArenaAt(memory_arena * Arena);
u32 ArenaMax(memory_arena * Arena, u32 Align);

#define ArenaPush(Arena, type) ArenaPushBytes((Arena), sizeof(type), alignof(type))
#define ArenaPushArray(Arena, type, Count) ArenaPushBytes((Arena), sizeof(type) * (Count), alignof(type))
#define ArenaPushAndCopy(Arena, type, Data) ArenaPushBytesAndCopy((Arena), sizeof(type), alignof(type), (Data))
#define ArenaPushArrayAndCopy(Arena, type, Count, Data) ArenaPushBytesAndCopy((Arena), sizeof(type) * (Count), alignof(type), (Data))

void * ArenaPushBytes(memory_arena * Arena, u32 Size, u32 Align);
void * ArenaPushBytesAndCopy(memory_arena * Arena, u32 Size, u32 Align, void * Data);
void ArenaRewind(memory_arena * Arena, u32 Position);
void ArenaZero(memory_arena * Arena);

memory_arena_block * ArenaBlockAdd(memory_arena * Arena);
void ArenaBlockRelease(memory_arena * Arena, memory_arena_block * Block);
void * ArenaBlockAt(memory_arena_block * Block);

void * ArenaBlockPushBytes(memory_arena_block * Block, u32 Size, u32 Align);
void ArenaBlockRewind(memory_arena_block * Block, u32 BlockPosition);

// temp and scratch arenas

typedef struct temp_memory_arena
{
	memory_arena * Arena;
	u32 Position;
} temp_memory_arena;

temp_memory_arena TempArenaCreate(memory_arena * Arena);
void TempArenaRelease(temp_memory_arena TempArena);

extern thread_var memory_arena * GlobalScratchMemoryArenaPool[10];
temp_memory_arena GetScratchArena(memory_arena ** Conflicts, u32 ConflictsCount);
#define ReleaseScratchArena(X) TempArenaRelease(X)

// buffers

typedef struct memory_buffer {
	void * Data;

	u32 MaxCount;
	u32 Count;
} memory_buffer;

memory_buffer BufferCreate(u32 Bytes);
void BufferCreateArray(u32 Bytes, u32 Count, memory_buffer * Result);
memory_buffer BufferCreateOnArena(u32 Bytes, memory_arena * Arena);

void BufferReset(memory_buffer * Buffer);
void BufferZero(memory_buffer * Buffer);
void * BufferAt(memory_buffer * Buffer);
void * BufferPush(memory_buffer * Buffer, void * Data, u32 Size);
void * BufferPushNoWrite(memory_buffer * Buffer, u32 Size);
void BufferPop(memory_buffer * Buffer, u32 Size);
void BufferPopTo(memory_buffer * Buffer, u32 Count);
u32 BufferLeft(memory_buffer * Buffer);

// scratch buffers

#define ScratchBufferSize Megabytes(1)

typedef struct scratch_buffer_pool_entry {
	memory_buffer Buffer;
	bool8 Occupied;
} scratch_buffer_pool_entry;

extern thread_var scratch_buffer_pool_entry GlobalScratchBufferPool[10];

memory_buffer * ScratchBufferStart();
void * ScratchBufferEnd(memory_buffer * ScratchBuffer, memory_arena * Arena, u32 Align);
void ScratchBufferRelease(memory_buffer * ScratchBuffer);

// ring buffer

typedef struct memory_ring {
	void * Data;

	u32 MaxCount;
	u32 ElementSize;

	u32 ReadPosition;
	u32 WritePosition;
} memory_ring;

memory_ring RingCreate(u32 ElementCount, u32 ElementSize);
u32 RingRead(memory_ring * Ring, void * Elements, u32 ElementCount);
u32 RingWrite(memory_ring * Ring, void * Elements, u32 ElementCount);

#ifdef __cplusplus
}
#endif

#endif