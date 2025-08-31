#ifndef base_types_h
#define base_types_h

#ifdef __cplusplus
extern "C" {
#endif

// general bits and bytes stuff

#include <stdint.h>
#include <stddef.h>

#define U8Max  0xff
#define U16Max 0xffff
#define U32Max 0xffffffff
#define U64Max 0xffffffffffffffffLL

#define I8Max  0x7f
#define I16Max 0x7fff
#define I32Max 0x7fffffff
#define I64Max 0x7fffffffffffffffLL

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uintptr_t uptr;
typedef intptr_t iptr;

typedef float f32;
typedef double f64;

typedef u8 byte;

typedef u8 bool8;
typedef u16 bool16;
typedef u32 bool32;
#define true ((bool8) 1)
#define false ((bool8) 0)

#ifdef COMPILER_MSVC
typedef u64 max_align_t;
#endif

#define OpaqueData(Name, Bytes) max_align_t Name[Bytes / sizeof(max_align_t)]
typedef OpaqueData(opaque_data_128b, 128);
typedef OpaqueData(opaque_data_256b, 256);
typedef OpaqueData(opaque_data_512b, 512);
typedef OpaqueData(opaque_data_1kb, Kilobytes(1));
typedef OpaqueData(opaque_data_2kb, Kilobytes(2));
#undef OpaqueData

#define GetHighDWord(QWord) ((u32)((u64)(QWord) >> 32))
#define GetLowDWord(QWord) ((u32)(u64)(QWord))
#define MakeQWord(LowDWord, HighDWord) (((u64)(u32)(LowDWord)) | ((u64)(u32)(HighDWord) << 32))

#define GetHighWord(DWord) ((u16)((u32)(DWord) >> 16))
#define GetLowWord(DWord) ((u16)(u32)(DWord))
#define MakeDWord(LowWord, HighWord) (((u32)(u16)(LowWord)) | ((u32)(u16)(HighWord) << 16))

#define GetHighByte(Word) ((u8)((u16)(Word) >> 8))
#define GetLowByte(Word) ((u8)(u16)(Word))
#define MakeWord(LowByte, HighByte) (((u16)(LowByte & 0xff)) | ((u16)(HighByte & 0xff) << 8))

void SetFlag(u8 * Flags, u8 FlagBit);
void UnsetFlag(u8 * Flags, u8 FlagBit);
void ToggleFlag(u8 * Flags, u8 FlagBit);
bool32 GetFlag(u8 * Flags, u8 FlagBit);

u16 SwapByteOrderU16(u16 Input);
u32 SwapByteOrderU32(u32 Input);
u64 SwapByteOrderU64(u64 Input);
void SwapByteOrderU128(u64 * A, u64 * B);

#ifdef __cplusplus
}
#endif

#endif