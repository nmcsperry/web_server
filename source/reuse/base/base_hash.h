#ifndef base_hash_h
#define base_hash_h

#ifdef __cplusplus
extern "C" {
#endif

typedef u32 hash_value;

hash_value HashStart();
hash_value HashStr8(str8 Value);
hash_value HashContinueStr8(hash_value Current, str8 Value);
hash_value HashStr8IgnoreCase(str8 Value);
hash_value HashContinueStr8IgnoreCase(hash_value Current, str8 Value);
hash_value HashContinueU8(hash_value Current, u8 Value);
hash_value HashContinueU16(hash_value Current, u16 Value);
hash_value HashContinueU32(hash_value Current, u32 Value);
hash_value HashContinueI8(hash_value Current, i8 Value);
hash_value HashContinueI16(hash_value Current, i16 Value);
hash_value HashContinueI32(hash_value Current, i32 Value);

typedef struct hash_table
{
	memory_arena * Arena;

	hash_value * Hashes;
	str8 * Keys;
	str8 * Data;

	u32 TableSize;
	u32 DataSize;
	u32 Count;
} hash_table;

typedef struct hash_slot_info
{
	u32 NaturalSlot;
	hash_value Hash;
	u32 Slot;
	bool32 Match;
	bool32 Error;
} hash_slot_info;

hash_table * HashTableCreate(memory_arena * Arena, u32 TableSize, u32 DataSize);
void * HashTableInsert(hash_table * HashTable, str8 Key, blob Data);
void * HashTableInsertPtr(hash_table * HashTable, str8 Key, void * Data);
bool32 HashTableDelete(hash_table * HashTable, str8 Key);
blob HashTableGet(hash_table * HashTable, str8 Key);
void * HashTableGetPtr(hash_table * HashTable, str8 Key);

#ifdef __cplusplus
}
#endif

#endif