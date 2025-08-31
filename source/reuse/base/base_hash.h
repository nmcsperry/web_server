#ifndef base_hash_h
#define base_hash_h

#ifdef __cplusplus
extern "C" {
#endif

typedef u32 hash_value;

hash_value HashStart();
hash_value HashStr8(str8 Value);
hash_value HashContinueStr8(hash_value Current, str8 Value);
hash_value HashContinueU8(hash_value Current, u8 Value);
hash_value HashContinueU16(hash_value Current, u16 Value);
hash_value HashContinueU32(hash_value Current, u32 Value);
hash_value HashContinueI8(hash_value Current, i8 Value);
hash_value HashContinueI16(hash_value Current, i16 Value);
hash_value HashContinueI32(hash_value Current, i32 Value);

#ifdef __cplusplus
}
#endif

#endif