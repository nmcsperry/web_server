#include "../../../base/base_include.h"
#include "../../io_include.h"

HANDLE Win32Out_StdOutHandle = INVALID_HANDLE_VALUE;
HANDLE Win32Out_StdErrorHandle = INVALID_HANDLE_VALUE;

void StdOutput(str8 String)
{
	if (Win32Out_StdOutHandle == INVALID_HANDLE_VALUE)
	{
		Win32Out_StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	WriteFile(Win32Out_StdOutHandle, String.Data, String.Count, 0, 0);
}

void StdOutputError(str8 String)
{
	if (Win32Out_StdErrorHandle == INVALID_HANDLE_VALUE)
	{
		Win32Out_StdErrorHandle = GetStdHandle(STD_ERROR_HANDLE);
	}

	WriteFile(Win32Out_StdErrorHandle, String.Data, String.Count, 0, 0);
}