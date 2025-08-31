#ifndef base_os_time_h
#define base_os_time_h

#include "../base_types.h"

// seconds are seconds
// useconds are microseconds
// wseconds are windows ticks, 100 nanoseconds

// windows time or "filetime" is since January 1st, 1601
// unix time is since January 1st, 1970

u64 WindowsTimeWSec();

u64 UnixTimeUSec();

u64 UnixTimeSec();

#endif