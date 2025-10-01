#include "base_include.h"

// bits and bytes and stuff

u16 SwapByteOrderU16(u16 Input)
{
	return (Input & 0xff) << 8 | (Input & 0xff00) >> 8;
}

u32 SwapByteOrderU32(u32 Input)
{
	return (Input & 0xff) << 24 | (Input & 0xff00) << 8 | (Input & 0xff0000) >> 8 | (Input & 0xff000000) >> 24;
}

u64 SwapByteOrderU64(u64 Input)
{
	return (Input & 0xff) << 56 | (Input & 0xff00) << 40 | (Input & 0xff0000) >> 24 | (Input & 0xff000000) >> 8 |
		(Input & 0xff00000000) << 8 | (Input & 0xff0000000000) << 24 | (Input & 0xff000000000000) >> 40 | (Input & 0xff00000000000000) >> 56;
}

void SwapByteOrderU128(u64 * A, u64 * B)
{
	u64 TempA = *A;
	*A = SwapByteOrderU64(*B);
	*B = SwapByteOrderU64(TempA);
}

u32 LeftRotate(u32 Value, u32 Amount)
{
	return Value << Amount | Value >> (32 - Amount);
}

u32 RightRotate(u32 Value, u32 Amount)
{
	return Value >> Amount | Value << (32 - Amount);
}

// todo: maybe add stuff like these:
// SwapByteOrderToLittleEndianU16() 
// SwapByteOrderToLittleEndianU32()
// SwapByteOrderToLittleEndianU64()
// SwapByteOrderToLittleEndianU128()
// SwapByteOrderFromLittleEndianU16()
// SwapByteOrderFromLittleEndianU32()
// SwapByteOrderFromLittleEndianU64()
// SwapByteOrderFromLittleEndianU128()
// SwapByteOrderToBigEndianU16()
// SwapByteOrderToBigEndianU32()
// SwapByteOrderToBigEndianU64()
// SwapByteOrderToBigEndianU128()
// SwapByteOrderFromBigEndianU16()
// SwapByteOrderFromBigEndianU32()
// SwapByteOrderFromBigEndianU64()
// SwapByteOrderFromBigEndianU128()

// todo: make these be macros?

void SetFlag(u8 * Flags, u8 FlagBit)
{
	*Flags |= FlagBit;
}

void UnsetFlag(u8 * Flags, u8 FlagBit)
{
	*Flags &= ~FlagBit;
}

void ToggleFlag(u8 * Flags, u8 FlagBit)
{
	*Flags ^= FlagBit;
}

bool32 GetFlag(u8 * Flags, u8 FlagBit)
{
	return !!(*Flags & FlagBit);
}