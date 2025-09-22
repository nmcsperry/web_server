#include "../base/base_include.h"
#include "io_include.h"

str8 FileInputFilename(char * Filename, memory_arena * Arena)
{
	file_handle File = FileOpenInput(Filename);

	str8 Result = FileInput(File, Arena);

	FileClose(File);

	return Result;
}

void FileOutputFilename(char * Filename, str8 String)
{
	file_handle File = FileOpenOutput(Filename, true);
	FileOutput(File, String);
	FileClose(File);
}

/* str8 FileInputFilenameStr8(str8 Filename, memory_arena * Arena)
{
	memory_buffer * Scratch = ScratchBufferStart();
	Str8WriteStr8(Scratch, Filename);
	Str8WriteChar8(Scratch, '\0');

	str8 Result = FileInputFilename(Str8FromBuffer(Scratch).Data, Arena);

	ScratchBufferRelease(Scratch);

	return Result;
} */