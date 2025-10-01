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

#if 0

// hash table
// this is not well tested

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

hash_table * HashTableCreate(memory_arena * Arena, u32 TableSize)
{
	hash_table * Result = ArenaPushZero(Arena, hash_table);
	Result->Arena = Arena;
	Result->Hashes = ArenaPushArrayZero(Arena, hash_value, TableSize);
	Result->Keys = ArenaPushArrayZero(Arena, str8, TableSize);
	Result->Data = ArenaPushArrayZero(Arena, str8, TableSize);

	Result->TableSize = TableSize;

	return Result;
}

bool32 HashTableInsert(hash_table * HashTable, str8 Key, void * Data)
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

void * HashTableGet(hash_table * HashTable, str8 Key)
{
	hash_slot_info SlotInfo = HashTableGetSlot(HashTable, Key);

	if (SlotInfo.Error || !SlotInfo.Match) return 0;

	return HashTable->Data[SlotInfo.Slot];
}

#else

hash_table * HashTableCreate(memory_arena * Arena, u32 TableSize)
{
	hash_table * HashTable = ArenaPush(Arena, hash_table);
	HashTable->TableSize = TableSize;
	HashTable->Data = ArenaPushArray(Arena, hash_table_node *, TableSize);
	HashTable->Arena = Arena;

	return HashTable;
}

bool32 HashTableInsert(hash_table * HashTable, str8 Key, void * Data)
{
	hash_value Hash = HashStr8(Key);
	u32 Slot = Hash % HashTable->TableSize;

	hash_table_node * Node;
	for (Node = HashTable->Data[Slot]; Node; Node = Node->Next)
	{
		if (Node->Hash == Hash && Str8Match(Node->Key, Key, 0))
		{
			break;
		}
	}

	if (Node == 0)
	{
		hash_table_node * NewNode = ArenaPush(HashTable->Arena, hash_table_node);
		SLLStackPush(HashTable->Data[Slot], NewNode);

		NewNode->Data = Data;
		NewNode->Hash = Hash;
		NewNode->Key = Key;
	}

	return false;
}

void * HashTableGet(hash_table * HashTable, str8 Key)
{
	hash_value Hash = HashStr8(Key);
	u32 Slot = Hash % HashTable->TableSize;

	hash_table_node * Node;
	for (Node = HashTable->Data[Slot]; Node; Node = Node->Next)
	{
		if (Node->Hash == Hash && Str8Match(Node->Key, Key, 0))
		{
			break;
		}
	}

	if (Node == 0) return 0;
	return Node->Data;
}

#endif

sha1 CalculateSHA1Core(sha1 Hash, u32 * Chunk)
{
	u32 W[80] = { 0 };
	// OSCopyMemory(W, Chunk, sizeof(u32) * 16);

	u32 A = Hash.E[0];
	u32 B = Hash.E[1];
	u32 C = Hash.E[2];
	u32 D = Hash.E[3];
	u32 E = Hash.E[4];

	for (i32 I = 0; I < 16; I++)
	{
		W[I] = SwapByteOrderU32(Chunk[I]);
	}

	for (i32 I = 16; I < 80; I++)
	{
		W[I] = LeftRotate(W[I - 3] ^ W[I - 8] ^ W[I - 14] ^ W[I - 16], 1);
	}

	for (i32 I = 0; I < 80; I++)
	{
		u32 F = 0, K = 0;
		if (I < 20)
		{
			F = (B & C) | ((~B) & D);
			K = 0x5A827999;
		}
		else if (I < 40)
		{
			F = B ^ C ^ D;
			K = 0x6ED9EBA1;
		}
		else if (I < 60)
		{
			F = B & C | B & D | C & D;
			K = 0x8F1BBCDC;
		}
		else if (I < 80)
		{
			F = B ^ C ^ D;
			K = 0xCA62C1D6;
		}

		u32 Temp = LeftRotate(A, 5) + F + E + K + W[I];
		E = D;
		D = C;
		C = LeftRotate(B, 30);
		B = A;
		A = Temp;
	}

	Hash.E[0] += A;
	Hash.E[1] += B;
	Hash.E[2] += C;
	Hash.E[3] += D;
	Hash.E[4] += E;

	return Hash;
}

sha1 CalculateSHA1(blob Message)
{
	u64 MessageLength = Message.Count * 8;

	sha1 Hash = {
		.E = {
			0x67452301,
			0xEFCDAB89,
			0x98BADCFE,
			0x10325476,
			0xC3D2E1F0
		}
	};

	while (Message.Count >= 64)
	{
		Hash = CalculateSHA1Core(Hash, Message.Data);

		Message.Count -= 64;
		Message.Data += 64;
	}

	u32 ExtraChunk[16] = { 0 };
	u8 * ExtraChunkU8 = &ExtraChunk[0];
	u64 * ExtraChunkU64 = &ExtraChunk[0];

	OSCopyMemory(ExtraChunkU8, Message.Data, Message.Count);
	ExtraChunkU8[Message.Count] = 0x80;
	if (Message.Count >= 56)
	{
		Hash = CalculateSHA1Core(Hash, ExtraChunk);
		OSZeroMemory(ExtraChunk, sizeof(ExtraChunk));
	}

	ExtraChunkU64[7] = SwapByteOrderU64(MessageLength);
	Hash = CalculateSHA1Core(Hash, ExtraChunk);

	return Hash;
}