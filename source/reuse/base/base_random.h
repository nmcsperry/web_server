#ifndef base_random_h
#define base_random_h

#include "base_types.h"

typedef struct random_series
{
	u32 Value;
} random_series;

random_series RandomSeriesCreate(u32 Seed);

u32 RandomU32(random_series * Series);
u64 RandomU64(random_series * Series);
i32 RandomI32Range(random_series * Series, i32 Min, i32 Max);
bool32 RandomBool(random_series * Series);

void ShuffleI32Array(random_series * Series, i32 * Array, u32 Count);
void ShufflePtrArray(random_series * Series, void ** Array, u32 Count);

#endif
