#include "../../../base/base_include.h"
#include "../../io_include.h"

typedef HANDLE win32_file_handle;

file_handle Win32FileWrap(win32_file_handle WinHandle)
{
	if (WinHandle == INVALID_HANDLE_VALUE)
	{
		return (file_handle) { .OSDataPointer = INVALID_HANDLE_VALUE, .IsValid = false };
	}

	return (file_handle) { .OSDataPointer = WinHandle, .IsValid = true };
}

win32_file_handle Win32FileUnwrap(file_handle FileHandle)
{
	if (!FileHandle.IsValid)
	{
		return INVALID_HANDLE_VALUE;
	}

	return FileHandle.OSDataPointer;
}

file_handle FileOpenInput(char * Filename)
{
	win32_file_handle WinHandle = CreateFileA(
		Filename,
		GENERIC_READ,
		0,
		0,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0
	);
	return Win32FileWrap(WinHandle);
}

file_handle FileOpenOutput(char * Filename, bool32 Truncate)
{
	win32_file_handle WinHandle = CreateFileA(
		Filename,
		GENERIC_WRITE,
		0,
		0,
		Truncate ? CREATE_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);
	return Win32FileWrap(WinHandle);
}

void FileClose(file_handle FileHandle)
{
	CloseHandle(Win32FileUnwrap(FileHandle));
}

u64 FileGetSize(file_handle FileHandle)
{
	if (!FileHandle.IsValid)
	{
		return 0;
	}
	else
	{
		u64 FileSize;
		GetFileSizeEx(Win32FileUnwrap(FileHandle), (PLARGE_INTEGER) &(FileSize));
		return FileSize;
	}
}

str8 FileInputSegmentToPtr(file_handle File, void * Memory, u32 Offset, u32 Count)
{
	u64 FileSize = FileGetSize(File);

	if (FileSize - Offset < Count)
	{
		return Str8Empty();
	}
	else
	{
		win32_file_handle WinHandle = Win32FileUnwrap(File);
		u32 BytesRead = 0;
		SetFilePointer(WinHandle, Offset, 0, FILE_BEGIN); // todo: support larger offsets
		ReadFile(WinHandle, Memory, Count, &BytesRead, 0);

		return (str8) { .Data = Memory, .Count = BytesRead };
	}
}

str8 FileInputToPtr(file_handle File, void * Memory, u32 MaxCount)
{
	u64 FileSize = FileGetSize(File);

	if (FileSize <= MaxCount - 1)
	{
		win32_file_handle WinHandle = Win32FileUnwrap(File);
		u32 BytesRead = 0;
		ReadFile(WinHandle, Memory, MaxCount, &BytesRead, 0);

		((u8 *) Memory)[BytesRead] = '\0';
	
		return (str8) { .Data = Memory, .Count = BytesRead };
	}
	else
	{
		return Str8Empty();
	}
}

str8 FileInput(file_handle File, memory_arena * Arena)
{
	u64 MaxCountU64 = ArenaMax(Arena, 32);
	u32 MaxCount = MaxCountU64 > U32Max ? U32Max : MaxCountU64;

	u64 FileSize = FileGetSize(File);

	if (MaxCount > 30000) MaxCount = 30000;

	if (FileSize <= MaxCount - 1)
	{
		win32_file_handle WinHandle = Win32FileUnwrap(File);

		void * Memory = ArenaPushBytes(Arena, FileSize + 1, 32);

		u32 BytesRead = 0;
		ReadFile(WinHandle, Memory, MaxCount, &BytesRead, 0);

		((u8 *) Memory)[BytesRead] = '\0';

		DWORD Error = GetLastError();
	
		str8 Result = (str8) { .Data = Memory, .Count = FileSize };

		return Result;
	}
	else
	{
		return Str8Empty();
	}
}

str8 FileInputSegment(file_handle File, memory_arena * Arena, u32 Offset, u32 Size)
{
	u32 FileSize = FileGetSize(File);

	if (Offset > FileSize)
	{
		return Str8Empty();
	}

	if (FileSize - Offset < Size)
	{
		Size = FileSize - Offset;
	}

	win32_file_handle WinHandle = Win32FileUnwrap(File);

	void * Memory = ArenaPushBytes(Arena, Size + 1, 32);

	SetFilePointer(WinHandle, Offset, 0, FILE_BEGIN); // todo: support larger offsets
	
	u32 BytesRead = 0;
	ReadFile(WinHandle, Memory, Size, &BytesRead, 0);
	((u8 *) Memory)[BytesRead] = '\0';

	return (str8) { .Data = Memory, .Count = BytesRead };
}

void FileOutputSegment(file_handle File, str8 String, u32 Offset)
{
	win32_file_handle WinHandle = Win32FileUnwrap(File);
	SetFilePointer(WinHandle, Offset, 0, FILE_BEGIN); // todo: support larger offsets
	WriteFile(WinHandle, String.Data, String.Count, 0, 0);
}

void FileOutput(file_handle File, str8 String)
{
	win32_file_handle WinHandle = Win32FileUnwrap(File);
	WriteFile(WinHandle, String.Data, String.Count, 0, 0);
}