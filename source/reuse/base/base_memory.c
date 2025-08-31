#include "base_include.h"

void * OSAllocateMemory(u32 Bytes)
{
	void * Result = OSReserveMemory(Bytes);
	OSCommitMemory(Result, Bytes);
	return Result;
}

void OSFreeMemory(void * Memory, u32 Bytes)
{
	OSDecommitMemory(Memory, Bytes);
	OSReleaseMemory(Memory, Bytes);
}

// arena

memory_arena * ArenaCreateAdv(u32 MaxSize, u32 CommitSize, u32 BlockSize)
{
	bool32 ProgressiveCommit = CommitSize != 0;
	bool32 ProgressiveBlockReserve = BlockSize != 0;
	if (!ProgressiveBlockReserve) BlockSize = MaxSize;
	if (!ProgressiveCommit) CommitSize = BlockSize;

	if (MaxSize == 0 || MaxSize < CommitSize || MaxSize < BlockSize || BlockSize < CommitSize || CommitSize < MINIMUM_ARENA_SIZE)
	{
		return 0;
	}

	if (MaxSize % BlockSize || CommitSize % MINIMUM_ARENA_SIZE || BlockSize % MINIMUM_ARENA_SIZE || MaxSize % MINIMUM_ARENA_SIZE)
	{
		return 0;
	}

	memory_arena_block * Block = OSReserveMemory(BlockSize);
	OSCommitMemory(Block, CommitSize);

	Block->Offset = 0;
	Block->MaxCount = BlockSize;
	Block->Count = sizeof(memory_arena_block);
	Block->CommitCount = CommitSize;

	Block->Next = 0;
	Block->Arena = 0;

	memory_arena * Arena = ArenaBlockPushBytes(Block, sizeof(memory_arena), alignof(memory_arena));

	Arena->First = Block;
	Arena->Last = Block;

	Arena->BlockSize = BlockSize;
	Arena->CommitSize = CommitSize;
	Arena->MaxCount = Block->MaxCount;
	Arena->Count = Block->Count;

	Arena->Flags = (ProgressiveCommit ? MemoryArenaFlag_ProgressiveCommit : 0) |
		(ProgressiveBlockReserve ? MemoryArenaFlag_ProgressiveBlockReserve : 0);

	Block->Arena = Arena;

	return Arena;
}

memory_arena * ArenaCreate(u32 Bytes)
{
	if (!Bytes)
	{
		Bytes = Megabytes(64);
	}

	return ArenaCreateAdv(Bytes, DEFAULT_ARENA_COMMIT_SIZE, 0);
}

void ArenaReset(memory_arena * Arena)
{
	ArenaRewind(Arena, ARENA_BASE_SIZE);
}

void * ArenaAt(memory_arena * Arena)
{
	return ArenaBlockAt(Arena->Last);
}

u32 ArenaMax(memory_arena * Arena, u32 Align)
{
	if (Align == 0)
	{
		Align = 1;
	}

	if (Arena->Last->Offset < Arena->MaxCount)
	{
		u32 AlignSize = AlignPow2(Arena->Last->Count, Align) - Arena->Last->Count;
		return Arena->Last->MaxCount - Arena->Last->Count - AlignSize;
	}
	else
	{
		return Arena->BlockSize;
	}
}

void * ArenaPushBytes(memory_arena * Arena, u32 Size, u32 Align)
{
	if (Align == 0)
	{
		Align = 1;
	}

	u32 AllocationSize = AlignPow2(Arena->Last->Count, Align) - Arena->Last->Count + Size;

	if (Arena->Last->MaxCount - Arena->Last->Count < AllocationSize)
	{
		u32 BlockWaste = Arena->Last->MaxCount - Arena->Last->Count;

		if (Arena->MaxCount - Arena->Count < Size + BlockWaste || Arena->BlockSize < Size)
		{
			return 0;
		}

		AllocationSize = Size;
		Arena->Count += BlockWaste;
		ArenaBlockAdd(Arena);
	}

	Arena->Count += AllocationSize;
	return ArenaBlockPushBytes(Arena->Last, Size, Align);
}

void * ArenaPushBytesAndCopy(memory_arena * Arena, u32 Size, u32 Align, void * Data)
{
	void * Pointer = ArenaPushBytes(Arena, Size, Align);

	if (Pointer == 0)
	{
		return 0;
	}

	OSCopyMemory(Pointer, Data, Size);

	return Pointer;
}

void ArenaRewind(memory_arena * Arena, u32 Position)
{
	memory_arena_block * Block = Arena->Last;
	while (Block->Offset > Position)
	{
		Block = Block->Previous;
	}
	memory_arena_block * UnneededBlock = Arena->Last;
	while (UnneededBlock != Block)
	{
		memory_arena_block * Previous = UnneededBlock->Previous;
		ArenaBlockRelease(Arena, UnneededBlock);
		UnneededBlock = Previous;
	}

	ArenaBlockRewind(Block, Position - Block->Offset);
	Arena->Count = Position;
}

void ArenaZero(memory_arena * Arena)
{
	memory_arena_block * Block = Arena->First;
	while (Block)
	{
		byte * Data = (void *)Block;
		u32 SaveSize = sizeof(memory_arena_block);
		if (Block == Arena->First)
		{
			SaveSize = ARENA_BASE_SIZE;
		}
		OSZeroMemory(Data + SaveSize, Block->CommitCount - SaveSize);
		Block = Block->Next;
	}
}

memory_arena_block * ArenaBlockAdd(memory_arena * Arena)
{
	memory_arena_block * Block = OSReserveMemory(Arena->BlockSize);
	OSCommitMemory(Block, Arena->CommitSize);

	Block->Offset = Arena->Last->Offset + Arena->BlockSize;
	Block->MaxCount = Arena->BlockSize;
	Block->Count = sizeof(memory_arena_block);
	Block->CommitCount = Arena->CommitSize;

	Block->Arena = Arena;

	Block->Next = 0;
	Block->Previous = 0;
	DLLPushBack(Arena->First, Arena->Last, Block);

	return Block;
}

void ArenaBlockRelease(memory_arena * Arena, memory_arena_block * Block)
{
	DLLRemove(Arena->First, Arena->Last, Block);

	OSDecommitMemory(Block, Block->CommitCount);
}

void * ArenaBlockAt(memory_arena_block * Block)
{
	return Block + Block->Count;
}

void * ArenaBlockPushBytes(memory_arena_block * Block, u32 Size, u32 Align)
{
	u32 AlignSize = AlignPow2(Block->Count, Align) - Block->Count;

	u32 NextCount = Block->Count + AlignSize + Size;
	if (NextCount > Block->CommitCount)
	{
		u32 CommitUpTo = Min(AlignPow2(NextCount, Block->Arena->CommitSize), Block->MaxCount);
		OSCommitMemory((byte *) Block + Block->CommitCount, CommitUpTo - Block->CommitCount);
		Block->CommitCount = CommitUpTo;
	}

	void * Result = (byte *) Block + Block->Count + AlignSize;
	Block->Count += AlignSize + Size;
	return Result;
}

void ArenaBlockRewind(memory_arena_block * Block, u32 BlockPosition)
{
	u32 CommitUpTo = AlignPow2(BlockPosition, Block->Arena->CommitSize);
	if (Block->CommitCount > CommitUpTo)
	{
		OSDecommitMemory((byte *) Block + CommitUpTo, Block->CommitCount - CommitUpTo);
	}
	Block->CommitCount = CommitUpTo;
}

// temp arena

temp_memory_arena TempArenaCreate(memory_arena * Arena)
{
	return (temp_memory_arena) {
		.Arena = Arena,
		.Position = Arena->Count
	};
}

void TempArenaRelease(temp_memory_arena TempArena)
{
	ArenaRewind(TempArena.Arena, TempArena.Position);
}

// scratch arena

thread_var memory_arena * GlobalScratchArenaPool[4] = { 0 };

void ScratchArenaInit()
{
	for (u32 ScratchIndex = 0; ScratchIndex < ArrayCount(GlobalScratchArenaPool); ScratchIndex++)
	{
		GlobalScratchArenaPool[ScratchIndex] = ArenaCreate(Megabytes(1));
	}
}

temp_memory_arena GetScratchArena(memory_arena ** Conflicts, u32 ConflictsCount)
{
	if (GlobalScratchArenaPool[0] == 0)
	{
		ScratchArenaInit();
	}

	memory_arena * ScratchArena = 0;
	for (u32 ScratchIndex = 0; ScratchIndex < ArrayCount(GlobalScratchArenaPool); ScratchIndex++)
	{
		ScratchArena = GlobalScratchArenaPool[ScratchIndex];
		for (u32 ConflictIndex = 0; ConflictIndex < ConflictsCount; ConflictIndex++)
		{
			memory_arena * Conflict = Conflicts[ConflictIndex];
			if (ScratchArena == Conflict)
			{
				continue;
			}
		}

		if (ScratchArena)
		{
			break;
		}
	}

	if (ScratchArena)
	{
		ArenaReset(ScratchArena);
	}
	else
	{
		return (temp_memory_arena) {0};
	}

	return TempArenaCreate(ScratchArena);
}

#define ReleaseScratchArena(X) TempArenaRelease(X)

// buffer, this is for using blocks of memory not in an arena

memory_buffer BufferCreate(u32 Bytes)
{
	memory_buffer Result = { 0 };
	Result.Data = OSAllocateMemory(Bytes);
	Result.MaxCount = Bytes;
	Result.Count = 0;

	return Result;
}

void BufferCreateArray(u32 Bytes, u32 Count, memory_buffer * Result)
{
	u8 * Memory = OSAllocateMemory(Bytes * Count);

	for (u32 I = 0; I < Count; I++)
	{
		Result[I].Data = Memory + I * Bytes;
		Result[I].MaxCount = Bytes;
		Result[I].Count = 0;
	}
}

memory_buffer BufferCreateOnArena(u32 Bytes, memory_arena * Arena)
{
	void * Data = ArenaPushBytes(Arena, Bytes, Kilobytes(1));
	return (memory_buffer)
	{
		.Data = Data,
		.MaxCount = Bytes,
		.Count = 0
	};
}

void BufferReset(memory_buffer * Buffer)
{
	if (Buffer == 0) return;
	Buffer->Count = 0;
}

void BufferZero(memory_buffer * Buffer)
{
	OSZeroMemory(Buffer->Data, Buffer->MaxCount);
}

void * BufferPush(memory_buffer * Buffer, void * Data, u32 Bytes)
{
	if (Bytes > Buffer->MaxCount - Buffer->Count)
	{
		return 0;
	}

	void * Dest = BufferAt(Buffer);
	OSCopyMemory(Dest, Data, Bytes);
	Buffer->Count += Bytes;

	return Dest;
}

void * BufferPushNoWrite(memory_buffer * Buffer, u32 Bytes)
{
	if (Bytes > Buffer->MaxCount - Buffer->Count)
	{
		return 0;
	}

	void * Dest = BufferAt(Buffer);
	Buffer->Count += Bytes;

	return Dest;
}

void BufferPop(memory_buffer * Buffer, u32 Bytes)
{
	if (Buffer->Count < Bytes)
	{
		Buffer->Count = 0;
	}
	else
	{
		Buffer->Count -= Bytes;
	}
}

void BufferPopTo(memory_buffer * Buffer, u32 Bytes)
{
	if (Bytes < Buffer->Count)
	{
		Buffer->Count = Bytes;
	}
}

void * BufferAt(memory_buffer * Buffer)
{
	return (byte *) Buffer->Data + Buffer->Count;
}

u32 BufferLeft(memory_buffer * Buffer)
{
	return Buffer->MaxCount - Buffer->Count;
}

void BufferFree(memory_buffer * Buffer)
{
	OSFreeMemory(Buffer->Data, Buffer->MaxCount);

	Buffer->Count = 0;
	Buffer->Data = 0;
	Buffer->MaxCount = 0;
}

// scratch buffer, this is for building things in guaranteed continuous memory

thread_var scratch_buffer_pool_entry GlobalScratchBufferPool[10] = { 0 };

void ScratchBufferInit()
{
	for (u32 ScratchIndex = 0; ScratchIndex < ArrayCount(GlobalScratchArenaPool); ScratchIndex++)
	{
		GlobalScratchBufferPool[ScratchIndex].Buffer = BufferCreate(ScratchBufferSize);
	}
}

memory_buffer * ScratchBufferStart()
{
	if (GlobalScratchBufferPool[0].Buffer.Data == 0)
	{
		ScratchBufferInit();
	}

	for (i32 ScratchBufferIndex = 0; ScratchBufferIndex < 10; ScratchBufferIndex++)
	{
		if (!GlobalScratchBufferPool[ScratchBufferIndex].Occupied)
		{
			GlobalScratchBufferPool[ScratchBufferIndex].Occupied = true;

			return &GlobalScratchBufferPool[ScratchBufferIndex].Buffer;
		}
	}

	return 0;
}

void * ScratchBufferEnd(memory_buffer * ScratchBuffer, memory_arena * Arena, u32 Align)
{
	if (ScratchBuffer == 0) return 0;

	if (Arena != 0)
	{
		void * Result = ArenaPushBytesAndCopy(Arena, ScratchBuffer->Count, Align, ScratchBuffer->Data);
		ScratchBufferRelease(ScratchBuffer);
		return Result;
	}
	else
	{
		ScratchBufferRelease(ScratchBuffer);
		return 0;
	}
}

void ScratchBufferRelease(memory_buffer * ScratchBuffer)
{
	if (ScratchBuffer == 0) return;

	ScratchBuffer->Count = 0;

	for (i32 ScratchBufferIndex = 0; ScratchBufferIndex < 10; ScratchBufferIndex++)
	{
		if (&GlobalScratchBufferPool[ScratchBufferIndex].Buffer == ScratchBuffer)
		{
			GlobalScratchBufferPool[ScratchBufferIndex].Occupied = false;
		}
	}
}

/*i32 BufferCount = 0;
	for (i32 BufferIndex = 0; BufferIndex < ArrayCount(GlobalScratchBufferPool); BufferIndex++)
	{
	if (GlobalScratchBufferPool[BufferIndex].Occupied)
	{
	BufferCount++;
	}
	}
	WriteOutFmt("For anyone interested, there are %{i32} buffers being used!\n", BufferCount);*/

// ring buffer

memory_ring RingCreate(u32 ElementCount, u32 ElementSize)
{
	memory_ring Result = { 0 };
	Result.ElementSize = ElementSize;
	Result.MaxCount = ElementCount;
	Result.Data = OSAllocateMemory(ElementCount * ElementSize);
	Result.ReadPosition = 0;
	Result.WritePosition = 0;

	return Result;
}

u32 RingRead(memory_ring * Ring, void * Elements, u32 ElementCount)
{
	i32 Diff = Ring->ReadPosition - Ring->WritePosition;
	u32 ElementsAvailable = Diff <= 0 ? Diff * -1 : Ring->MaxCount - Diff;

	if (ElementCount > ElementsAvailable) ElementCount = ElementsAvailable;

	if (Elements)
	{
		u32 FirstPartElemCount = Min(Ring->MaxCount - Ring->ReadPosition, ElementCount);
		void * FirstPartReadPosition = (byte *) Ring->Data + Ring->ElementSize * Ring->ReadPosition;
		OSCopyMemory(Elements, FirstPartReadPosition, Ring->ElementSize * FirstPartElemCount);

		i32 SecondPartElemCount = ElementCount - FirstPartElemCount;
		if (SecondPartElemCount > 0)
		{
			void * SecondPartWritePosition = (byte *) Elements + Ring->ElementSize * FirstPartElemCount;
			OSCopyMemory(SecondPartWritePosition, Ring->Data, Ring->ElementSize * SecondPartElemCount);
		}
	}

	Ring->ReadPosition = (Ring->ReadPosition + ElementCount) % Ring->MaxCount;
	return ElementCount;
}

u32 RingWrite(memory_ring * Ring, void * Elements, u32 ElementCount)
{
	i32 Diff = Ring->ReadPosition - Ring->WritePosition;
	u32 EmptySlotsAvailable = Diff <= 0 ? Ring->MaxCount + Diff : Diff;

	if (ElementCount > EmptySlotsAvailable) ElementCount = EmptySlotsAvailable;

	u32 FirstPartElemCount = Min(Ring->MaxCount - Ring->WritePosition, ElementCount);
	void * FirstPartWritePosition = (byte *) Ring->Data + Ring->ElementSize * Ring->WritePosition;
	OSCopyMemory(FirstPartWritePosition, Elements, Ring->ElementSize * FirstPartElemCount);

	i32 SecondPartElemCount = ElementCount - FirstPartElemCount;
	if (SecondPartElemCount > 0)
	{
		void * SecondPartReadPosition = (byte *) Elements + Ring->ElementSize * FirstPartElemCount;
		OSCopyMemory(Ring->Data, SecondPartReadPosition, Ring->ElementSize * SecondPartElemCount);
	}

	Ring->WritePosition = (Ring->WritePosition + ElementCount) % Ring->MaxCount;
	return ElementCount;
}

