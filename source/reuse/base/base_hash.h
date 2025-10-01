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

#if 0

typedef struct hash_table
{
	memory_arena * Arena;

	hash_value * Hashes;
	str8 * Keys;
	void ** Data;

	u32 TableSize;
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

hash_table * HashTableCreate(memory_arena * Arena, u32 TableSize);
bool32 HashTableInsert(hash_table * HashTable, str8 Key, void * Data);
bool32 HashTableDelete(hash_table * HashTable, str8 Key);
void * HashTableGet(hash_table * HashTable, str8 Key);

#else

typedef struct hash_table_node hash_table_node;
struct hash_table_node
{
	str8 Key;
	hash_value Hash;

	void * Data;
	hash_table_node * Next;
};

typedef struct hash_table {
	memory_arena * Arena;

	hash_table_node ** Data;
	u32 TableSize;
} hash_table;

hash_table * HashTableCreate(memory_arena * Arena, u32 TableSize);
bool32 HashTableInsert(hash_table * HashTable, str8 Key, void * Data);
void * HashTableGet(hash_table * HashTable, str8 Key);

#endif

typedef struct sha1
{
	u32 E[5];
} sha1;

sha1 CalculateSHA1(blob Message);

#ifdef __cplusplus
}
#endif

#endif