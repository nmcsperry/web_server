#ifndef io_file_h
#define io_file_h

#include "io.h"

typedef struct file_handle
{
	bool8 IsValid;
	union {
		u64 OSDataU64;
		i64 OSDataI64;
		u32 OSDataU32[2];
		i32 OSDataI32[2];
		void * OSDataPointer;
	};
} file_handle;

str8 FileInputFilename(char * Filename, memory_arena * Arena);
void FileOutputFilename(char * Filename, str8 String);

#endif