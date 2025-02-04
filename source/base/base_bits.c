#include "base_include.h"

// todo: redo this whole file

void AdvanceBits(bit_str * BitString, u8 GetCount)
{
	if (GetCount > 64)
	{
		return; // todo: error
	}

	if (BitString->String.Count * 8 - BitString->BitsConsumed < GetCount)
	{
		return; // todo: error
	}

	BitString->String.Count -= (BitString->BitsConsumed + GetCount) / 8;
	if (BitString->Endianness == Endianness_LittleEndian)
	{
		BitString->String.Data += (BitString->BitsConsumed + GetCount) / 8;
	}
	BitString->BitsConsumed = (BitString->BitsConsumed + GetCount) % 8;
}

u64 PeekBits(bit_str * BitString, u8 GetCount)
{
	if (GetCount == 0)
	{
		return 0;
	}
 
	if (GetCount > 64)
	{
		return 0; // todo: error
	}

	if (BitString->String.Count * 8 - BitString->BitsConsumed < GetCount)
	{
		return 0; // todo: error
	}

	u64 Buffer = 0;
	u32 BufferBitCount = 0;

	i32 ByteIndex = BitString->Endianness == Endianness_LittleEndian ? 0 : BitString->String.Count - 1;
	i32 ByteDirection = BitString->Endianness == Endianness_LittleEndian ? 1 : -1;

	bool32 FirstLoop = true;
	while (BufferBitCount < GetCount)
	{
		u8 BitsConsumed = FirstLoop ? BitString->BitsConsumed : 0;
		u8 BitCount = GetCount - BufferBitCount;
		if (BitCount > 8 - BitsConsumed) BitCount = 8 - BitsConsumed;

		u8 Bits;
		if (ByteIndex < 0 || ByteIndex >= BitString->String.Count)
		{
			Bits = 0;
		}
		else
		{
			Bits = BitString->String.Data[ByteIndex];
		}

		u8 Mask = (1 << BitCount) - 1;
		if (BitString->BitOrder == BitString_LeastSignificantBitsFirst)
		{
			// 0b11111111   Eight bits
			// 0b00000111   Three bits, with no bits read yet
			// 0b00111xxx   Three bits, with three bits already read

			u8 MaskedBits = (Bits >> BitsConsumed) & Mask;
			Buffer |= MaskedBits << BufferBitCount;
		}
		else if (BitString->BitOrder == BitString_MostSignificantBitsFirst)
		{
			// 0b11111111   Eight bits
			// 0b11100000   Three bits, with no bits read yet
			// 0bxxx11100   Three bits, with three bits already read

			u8 BitOffset = 8 - BitCount - BitsConsumed;

			// if BitCount was too high, we would set: BitCount = 8 - BitsConsumed, so then:
			// BitOffset = 8 - BitCount - BitsConsumed
			// BitOffset = 8 - (8 - BitsConsumed) - BitsConsumed
			// BitOffset = 8 - 8 + BitsConsumed - BitsConsumed
			// BitOffset = 0 + 0 = 0
			// Which is correct; if we are going to the end of the byte, there is no shift.

			u8 MaskedBits = (Bits >> BitOffset) & Mask;

		
			// Start      | 00000
			// Add 2 bits | 11000 | They were shifted over 3 (5 - 2)
			// Add 1 bit  | 11100 | It was shifted over 2 (5 - 1 - 2)

			Buffer |= MaskedBits << (GetCount - BitCount - BufferBitCount);
		}

		// printf("\tGathered %u bits from byte %i\n", BitCount, ByteIndex);
		// printf("\tThe bit offset was %u!\n", BitOffset);

		BufferBitCount += BitCount;
		ByteIndex += ByteDirection;

		FirstLoop = false;
	}

	return Buffer;
}

u64 PopBits(bit_str * BitString, u8 BitCount)
{
	u64 Result = PeekBits(BitString, BitCount);
	AdvanceBits(BitString, BitCount);
	return Result;
}

i64 PopBitsSigned(bit_str * BitString, u8 BitCount)
{
	if (BitCount == 0) return 0;

	u64 Result = PeekBits(BitString, BitCount);
	if ((Result >> BitCount - 1) & 1)
	{
		Result |= ~((1 << BitCount) - 1);
	}

	AdvanceBits(BitString, BitCount);
	return Result;
}

u64 PopBytes(bit_str * BitString, u8 ByteCount)
{
	u64 Buffer = 0;

	AdvanceBitsToNextByte(BitString);
	for (u8 Byte = 0; Byte < ByteCount; Byte++)
	{
		Buffer |= BitString->String.Data[0] << (8 * Byte);
		BitString->String.Data++;
		BitString->String.Count--;
	}

	return Buffer;
}

u64 PopBytesSigned(bit_str * BitString, u8 ByteCount)
{
	u64 Value = PopBytes(BitString, ByteCount);
	i64 SignedValue;

	if (ByteCount == 1)
	{
		i8 * CorrectSize = (void *)&Value;
		SignedValue = *CorrectSize;
	}
	else if (ByteCount == 2)
	{
		i16 * CorrectSize = (void *)&Value;
		SignedValue = *CorrectSize;
	}
	else if (ByteCount == 4)
	{
		i32 * CorrectSize = (void *)&Value;
		SignedValue = *CorrectSize;
	}
	else if (ByteCount == 8)
	{
		i64 * CorrectSize = (void *)&Value;
		SignedValue = *CorrectSize;
	}
	else
	{
		// error
		SignedValue = 0;
	}

	return SignedValue;
}

void AdvanceBitsToNextByte(bit_str * BitString)
{
	u8 BitsLeft = 8 - BitString->BitsConsumed;
	if (BitsLeft == 8) return;
	AdvanceBits(BitString, BitsLeft);
}

u64 FlipBitOrder(u64 Bits, u8 Count)
{
	u64 Result = 0;
	for (i32 BitIndex = 0; BitIndex < Count; BitIndex++)
	{
		Result |= ((Bits >> BitIndex) & 1) << (Count - BitIndex - 1);
	}
	return Result;
}