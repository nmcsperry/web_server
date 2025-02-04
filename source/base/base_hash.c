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
	for (int CharIndex = 0; CharIndex < Value.Count; CharIndex++) {
		u8 CharValue = (u8) Value.Data[CharIndex];
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