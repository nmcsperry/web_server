#include "../../base/base_include.h"
#include "../../io/io_include.h"

#pragma pack(push,1)
typedef struct bitmap_file_header {
	u8 Magic[2];
	u32 Size;
	u16 Reserved1;
	u16 Reserved2;
	u32 Offset;
} bitmap_file_header;

typedef struct bitmap_windows_header {
	u32 DibHeaderSize;
	u32 Width;
	u32 Height;
	u16 ColorPlanesCount;
	u16 BitsPerPixel;
} bitmap_windows_header;
#pragma pack(pop)

typedef struct bitmap {
	u32 * Data;
	int Width;
	int Height;
} bitmap;

// byte Buffer[4 * 200 * 200];

bitmap LoadBitmapFromData(memory_arena * Memory, str8 Data)
{
	bitmap_file_header * BitmapHeader = (bitmap_file_header *) Data.Data;
	int Offset = BitmapHeader->Offset;

	bitmap_windows_header * WindowsHeader = (bitmap_windows_header *)(void *)(BitmapHeader + 1);
	bitmap Bitmap = { 0 };
	Bitmap.Width = WindowsHeader->Width;
	Bitmap.Height = WindowsHeader->Height;

	int Pitch = Bitmap.Width * 3;
	u32 Mask = 0xfffffffc;
	if (Bitmap.Width & ~Mask) {
		Pitch &= Mask;
		Pitch += 4;
	}

	u8 * BitmapData = (u8 *) Data.Data + Offset;

	Bitmap.Data = ArenaPushArray(Memory, u8, 4 * Bitmap.Width * Bitmap.Height);

	if (Bitmap.Data)
	{
		u8 * CurrentPixel = BitmapData;
		for (int Y = 0; Y < Bitmap.Height; Y++)
		{
			for (int X = 0; X < Bitmap.Width; X++)
			{
				u8 Red = CurrentPixel[(Y * Pitch + X * 3) + 2];
				u8 Green = CurrentPixel[(Y * Pitch + X * 3) + 1];
				u8 Blue = CurrentPixel[(Y * Pitch + X * 3) + 0];
				u8 Alpha = 255;

				i32_v3 Color = I32V3(Red, Green, Blue);
				if (I32V3Equals(Color, I32V3(255, 0, 255)))
				{
					U8V4Set(U8V4(0, 0, 0, 0), &Red, &Green, &Blue, &Alpha);
				}

				u32 Pixel = (Red) | (Green << 8) | (Blue << 16) | (Alpha << 24);
				Bitmap.Data[Y * Bitmap.Width + X] = Pixel;
			}
		}
	}

	return Bitmap;
}

bitmap LoadBitmapFromFile(memory_arena * Memory, char * Filename)
{
	temp_memory_arena FileMemory = GetScratchArena(&Memory, 1);
	str8 Data = FileInputFilename(Filename, FileMemory.Arena);

	bitmap Result = LoadBitmapFromData(Memory, Data);
	ReleaseScratchArena(FileMemory);

	return Result;
}