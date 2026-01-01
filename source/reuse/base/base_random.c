#include "base_include.h"

random_series RandomSeriesCreate(u32 Seed)
{
	if (!Seed)
	{
		Seed = (u32) UnixTimeUSec();
	}

	random_series Result = { 0 };
	Result.Value = Seed;

	return Result;
}

// todo: do something more proper than whatever this is...?
u32 RandomU32(random_series * Series)
{
	Series->Value ^= Series->Value << 13;
	Series->Value ^= Series->Value >> 17;
	Series->Value ^= Series->Value << 5;

	return Series->Value;
}

u64 RandomU64(random_series * Series)
{
	return MakeQWord(RandomU32(Series), RandomU32(Series));
}

i32 RandomI32Range(random_series * Series, i32 MinI, i32 MaxI)
{
	return RandomU32(Series) % (MaxI - MinI) + MinI;
}

bool32 RandomBool(random_series * Series)
{
	return RandomU32(Series) % 2;
}

void ShuffleI32Array(random_series * Series, i32 * Array, u32 Count)
{
	for (i32 Index = 0; Index < Count; Index++)
	{
		i32 SwapIndex = RandomU32(Series) % Count;
		i32 Temp = Array[SwapIndex];

		Array[SwapIndex] = Array[Index];
		Array[Index] = Temp;
	}
}

void ShufflePtrArray(random_series * Series, void ** Array, u32 Count)
{
	for (i32 Index = 0; Index < Count; Index++)
	{
		i32 SwapIndex = RandomU32(Series) % Count;
		void * Temp = Array[SwapIndex];

		Array[SwapIndex] = Array[Index];
		Array[Index] = Temp;
	}
}
