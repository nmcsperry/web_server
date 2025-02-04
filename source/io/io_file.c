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