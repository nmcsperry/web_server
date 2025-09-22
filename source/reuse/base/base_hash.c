#include "base_include.h"

// 64 bit: start:    0xcbf29ce484222325ULL
//         multiply: 0x100000001b3ULL

// 32 bit: start:    0x811c9dc5
//         multiply: 0x01000193

hash_value HashStart()
{
	return 0x811c9dc5;
}

hash_value HashStr8(str8 Value)
{
	return HashContinueStr8(HashStart(), Value);
}

hash_value HashContinueStr8(hash_value Current, str8 Value)
{
	for (u32 CharIndex = 0; CharIndex < Value.Count; CharIndex++)
	{
		u8 CharValue = (u8) Value.Data[CharIndex];
		Current = (Current ^ CharValue) * 0x01000193;
	}
	return Current;
}

hash_value HashStr8IgnoreCase(str8 Value)
{
	return HashContinueStr8(HashStart(), Value);
}

hash_value HashContinueStr8IgnoreCase(hash_value Current, str8 Value)
{
	for (u32 CharIndex = 0; CharIndex < Value.Count; CharIndex++)
	{
		u8 CharValue = (u8)Value.Data[CharIndex];
		if (CharValue >= 'A' || CharValue <= 'Z') CharValue += 'a' - 'A';
		Current = (Current ^ CharValue) * 0x01000193;
	}
	return Current;
}

hash_value HashContinueU8(hash_value Current, u8 Value)
{
	return (Current ^ Value) * 0x01000193;
}

hash_value HashContinueU32(hash_value Current, u32 Value)
{
	Current = (Current ^ (Value >> 0 & 0xff)) * 0x01000193;
	Current = (Current ^ (Value >> 8 & 0xff)) * 0x01000193;
	Current = (Current ^ (Value >> 16 & 0xff)) * 0x01000193;
	Current = (Current ^ (Value >> 24 & 0xff)) * 0x01000193;
	return Current;
}

hash_value HashContinueU16(hash_value Current, u16 Value)
{
	Current = (Current ^ (Value >> 0 & 0xff)) * 0x01000193;
	Current = (Current ^ (Value >> 8 & 0xff)) * 0x01000193;
	return Current;
}

hash_value HashContinueI8(hash_value Current, i8 Value)
{
	return HashContinueU8(Current, (u8) Value);
}

hash_value HashContinueI16(hash_value Current, i16 Value)
{
	return HashContinueU16(Current, (u16) Value);
}

hash_value HashContinueI32(hash_value Current, i32 Value)
{
	return HashContinueU32(Current, (u32) Value);
}

// hash table

hash_slot_info HashTableGetSlot(hash_table * HashTable, str8 Key)
{
	hash_value HashValue = HashStr8(Key);
	if (HashValue == 0) HashValue = 1;

	u32 NaturalSlot = HashValue % HashTable->TableSize;
	u32 CurrentSlot = NaturalSlot;

	hash_slot_info Result = { 0 };
	Result.NaturalSlot = NaturalSlot;
	Result.Hash = HashValue;

	// scan until we find matching hashes
	while (HashTable->Hashes[CurrentSlot] != HashValue && HashTable->Hashes[CurrentSlot] != 0)
	{
		CurrentSlot++;
		if (CurrentSlot >= HashTable->TableSize) CurrentSlot = 0;
		if (CurrentSlot == NaturalSlot)
		{
			Result.Error = true;
			return Result;
		}
	}

	// scan until we find matching key

	bool32 FoundMatch;
	while (!(FoundMatch = Str8Match(Key, HashTable->Keys[CurrentSlot], 0)) && HashTable->Hashes[CurrentSlot] == HashValue)
	{
		CurrentSlot++;
		if (CurrentSlot >= HashTable->TableSize) CurrentSlot = 0;
		if (CurrentSlot == NaturalSlot)
		{
			Result.Error = true;
			return Result;
		}
	}

	Result.Slot = CurrentSlot;
	Result.Match = FoundMatch;
	return Result;
}

hash_table * HashTableCreate(memory_arena * Arena, u32 TableSize, u32 DataSize)
{
	hash_table * Result = ArenaPushZero(Arena, hash_table);
	Result->Arena = Arena;
	Result->Hashes = ArenaPushArrayZero(Arena, hash_value, TableSize);
	Result->Keys = ArenaPushArrayZero(Arena, str8, TableSize);
	Result->Data = ArenaPushArrayZero(Arena, str8, TableSize);

	Result->TableSize = TableSize;
	Result->DataSize = DataSize;

	return Result;
}

bool32 HashTableInsert(hash_table * HashTable, str8 Key, blob Data)
{
	hash_slot_info SlotInfo = HashTableGetSlot(HashTable, Key);
	u32 Slot = SlotInfo.Slot;

	if (SlotInfo.Error)
	{
		return false;
	}

	if (SlotInfo.Match)
	{
		// replace value in existing slot
		HashTable->Data[Slot] = Data;

		return true;
	}
	else if (HashTable->Hashes[Slot] == 0)
	{
		// make room if needed
		if (HashTable->Count < HashTable->TableSize)
		{
			u32 EndSlot = Slot;
			while (HashTable->Hashes[EndSlot])
			{
				EndSlot++;
				if (EndSlot >= HashTable->TableSize) EndSlot = 0;
			}

			u32 Size = Slot - EndSlot;

			OSMoveMemory(&HashTable->Hashes[Slot], sizeof(hash_value) * Size, &HashTable->Hashes[Slot + 1]);
			OSMoveMemory(&HashTable->Data[Slot], sizeof(str8) * Size, &HashTable->Data[Slot + 1]);
			OSMoveMemory(&HashTable->Keys[Slot], sizeof(str8) * Size, &HashTable->Keys[Slot + 1]);

			OSZeroMemory(&HashTable->Hashes[Slot], sizeof(hash_value));
			OSZeroMemory(&HashTable->Data[Slot], sizeof(str8));
			OSZeroMemory(&HashTable->Keys[Slot], sizeof(str8));
		}

		// insert into new slot
		HashTable->Hashes[Slot] = SlotInfo.Hash;
		HashTable->Data[Slot] = Data;
		HashTable->Keys[Slot] = Key;

		HashTable->Count++;

		return true;
	}
	else
	{
		return false;
	}
}

bool32 HashTableInsertPtr(hash_table * HashTable, str8 Key, void * Data)
{
	if (HashTable->DataSize)
	{
		return HashTableInsert(HashTable, Key, (blob) { .Data = Data, .Count = HashTable->DataSize });
	}

	return false;
}

bool32 HashTableDelete(hash_table * HashTable, str8 Key)
{
	hash_slot_info SlotInfo = HashTableGetSlot(HashTable, Key);
	u32 Slot = SlotInfo.Slot;

	if (SlotInfo.Error || !SlotInfo.Match) return false;

	u32 EndSlot = Slot;
	while (HashTable->Hashes[EndSlot] % HashTable->TableSize != EndSlot)
	{
		EndSlot++;
		if (EndSlot >= HashTable->TableSize) EndSlot = 0;
	}

	u32 Size = Slot - EndSlot;

	OSMoveMemory(&HashTable->Hashes[Slot + 1], sizeof(hash_value) * Size, &HashTable->Hashes[Slot]);
	OSMoveMemory(&HashTable->Data[Slot + 1], sizeof(str8) * Size, &HashTable->Data[Slot]);
	OSMoveMemory(&HashTable->Keys[Slot + 1], sizeof(str8) * Size, &HashTable->Keys[Slot]);

	OSZeroMemory(&HashTable->Hashes[EndSlot], sizeof(hash_value));
	OSZeroMemory(&HashTable->Data[EndSlot], sizeof(str8));
	OSZeroMemory(&HashTable->Keys[EndSlot], sizeof(str8));

	HashTable->Count--;
}

blob HashTableGet(hash_table * HashTable, str8 Key)
{
	hash_slot_info SlotInfo = HashTableGetSlot(HashTable, Key);

	if (SlotInfo.Error || !SlotInfo.Match) return (blob) { 0 };

	return HashTable->Data[SlotInfo.Slot];
}

void * HashTableGetPtr(hash_table * HashTable, str8 Key)
{
	return HashTableGet(HashTable, Key).Data;
}