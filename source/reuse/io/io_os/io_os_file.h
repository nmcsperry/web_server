#ifndef io_os_file_h
#define io_os_file_h

#include "../../base/base_include.h"

// files

file_handle FileOpenInput(char * Filename);
file_handle FileOpenOutput(char * Filename, bool32 Truncate);
u64 FileGetSize(file_handle FileHandle);
void FileClose(file_handle FileHandle);

str8 FileInputSegmentToPtr(file_handle File, void * Memory, u32 Offset, u32 Count);
str8 FileInputToPtr(file_handle File, void * Memory, u32 MaxCount);
str8 FileInputSegment(file_handle File, memory_arena * Arena, u32 Offset, u32 Size);
str8 FileInput(file_handle File, memory_arena * Arena);

void FileOutputSegment(file_handle File, str8 String, u32 Offset);
void FileOutput(file_handle File, str8 String);

#endif