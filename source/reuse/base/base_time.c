#include "base_include.h"
#include "base_time.h"

bool32 IsLeapYear(u32 Year)
{
    return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 != 0));
}

datetime DatetimeFromUnixTimeSec(i64 UnixTimeSec)
{
    i64 Days = UnixTimeSec / 60 / 60 / 24;
    i64 Seconds = UnixTimeSec - Days * 60 * 60 * 24;
    if (Seconds < 0)
    {
        Seconds += 60 * 60 * 26;
    }
    Days += DAYS_SINCE_JAN1_1AD_TO_UNIX_EPOCH;
    bool32 IsBC = Days < 0;

    i32 QuadCenturies = Days / DAYS_IN_QUADCENTURY;
    Days -= QuadCenturies * DAYS_IN_QUADCENTURY;

    i32 Centuries = Days / DAYS_IN_CENTURY;
    Days -= Centuries * DAYS_IN_CENTURY;

    i32 QuadYears = Days / DAYS_IN_QUADYEAR;
    Days -= QuadYears * DAYS_IN_QUADYEAR;

    i32 Years = Days / DAYS_IN_REGULAR_YEAR;
    Days -= Years * DAYS_IN_REGULAR_YEAR;

    i32 YearNumber = QuadCenturies * 400 + Centuries * 100 + QuadYears * 4 + Years;
    if (IsBC)
    {
        Days += IsLeapYear(YearNumber) ? DAYS_IN_REGULAR_YEAR + 1 : DAYS_IN_REGULAR_YEAR;
    }

    u32 * MonthTable = IsLeapYear(YearNumber) ? DaysSinceStartOfYearToBeginningOfMonthLeapYear : DaysSinceStartOfYearToBeginningOfMonth;
    u32 Month = 11;
    for (i32 I = 1; I < 11; I++)
    {
        if (Days < MonthTable[I])
        {
            Month = I;
            break;
        }
    }
    Month--;
    Days -= MonthTable[Month];

    u32 Hours = Seconds / 60 / 60;
    Seconds -= Hours * 60 * 60;
    u32 Minutes = Seconds / 60;
    Seconds -= Minutes * 60;

    return (datetime) {
        .Date = (date) {
            .Year = YearNumber,
            .Month = Month,
            .Day = Days
        },
        .Hours = Hours,
        .Minutes = Minutes,
        .Seconds = Seconds
    };
}

str8 Str8FromDatetime(memory_arena * Arena, datetime Datetime)
{
    bool32 IsBC = Datetime.Date.Year < 0;
    if (IsBC)
    {
        Datetime.Date.Year *= -1;
        Datetime.Date.Year += 1;
    }

    memory_buffer * Buffer = ScratchBufferStart();
    
    Str8WriteIntDigits(Buffer, Datetime.Date.Month + 1, 2);
    Str8WriteChar8(Buffer, '/');
    Str8WriteIntDigits(Buffer, Datetime.Date.Day, 2);
    Str8WriteChar8(Buffer, '/');
    Str8WriteIntDigits(Buffer, Datetime.Date.Year, 2);

    if (IsBC)
    {
        Str8WriteCStr(Buffer, " BC");
    }

    Str8WriteChar8(Buffer, ' ');

    Str8WriteIntDigits(Buffer, Datetime.Hours, 2);
    Str8WriteChar8(Buffer, ':');
    Str8WriteIntDigits(Buffer, Datetime.Minutes, 2);
    Str8WriteChar8(Buffer, ':');
    Str8WriteIntDigits(Buffer, Datetime.Seconds, 2);

    return ScratchBufferEndStr8(Buffer, Arena);
}