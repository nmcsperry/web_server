/*

Endianness reminders

Big Endian
	Network byte order
	Matches more how we *write*
	Most significant byte at lowest memory address
	e.g. in 0x0f01, f is at [0], 1 is at [1]

Little Endian
	x86
	Matches more how number bases "work" (value * base ^ place_value, place_value increases as you move up)
	Most significant byte at highest memory address
	e.g. in 0x0f01, f is at [1], 1 is at [0]

	One benefit: pointer points to the least significant byte
		e.g. a u16 pointer to 0x0007 can be meaningfully cast to a u8 pointer to 0x07

*/

#ifndef base_bits_h
#define base_bits_h

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BitString_MostSignificantBitsFirst = 1,
	BitString_LeastSignificantBitsFirst = 0,
}; 

#define Endianness_LittleEndian 0
#define Endianness_BigEndian 1

typedef struct bit_str {
	blob String;
	u8 BitsConsumed;

	bool32 Endianness;
	bool32 BitOrder;
} bit_str;

u64 PeekBits(bit_str * BitString, u8 BitCount);
void AdvanceBits(bit_str * BitString, u8 GetCount);
u64 PopBits(bit_str * BitString, u8 GetCount);
i64 PopBitsSigned(bit_str * BitString, u8 BitCount);
void AdvanceBitsToNextByte(bit_str * BitString);

u64 FlipBitOrder(u64 Bits, u8 Count);

#ifdef __cplusplus
}
#endif

#endif