#include "../../base_include.h"

#define WSEC_TO_SEC 10000000LL
#define WSEC_TO_USEC 10LL
#define WIN_UNIX_WSEC_DIFF 116444736000000000LL

u64 WindowsTimeWSec()
{
	FILETIME Time;
	GetSystemTimeAsFileTime(&Time);

	u64 Result = MakeQWord(Time.dwLowDateTime, Time.dwHighDateTime);
	return Result;
}

u64 UnixTimeUSec()
{
	return (WindowsTimeWSec() - WIN_UNIX_WSEC_DIFF) / WSEC_TO_USEC;
}

u64 UnixTimeSec()
{
	u64 WindowsTime = WindowsTimeWSec();
	return (WindowsTime - WIN_UNIX_WSEC_DIFF) / WSEC_TO_SEC;
}