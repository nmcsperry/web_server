#ifndef base_time_h
#define base_time_h

typedef enum month {
    Month_January = 0,
    Month_February = 1,
    Month_March = 2,
    Month_April = 3,
    Month_May = 4,
    Month_June = 5,
    Month_July = 6,
    Month_August = 7,
    Month_September = 8,
    Month_October = 9,
    Month_November = 10,
    Month_December = 11
} month;

u32 DaysInMonth[] = {
    [Month_January] = 31,
    [Month_February] = 28,
    [Month_March] = 31,
    [Month_April] = 30,
    [Month_May] = 31,
    [Month_June] = 30,
    [Month_July] = 31,
    [Month_August] = 31,
    [Month_September] = 30,
    [Month_October] = 31,
    [Month_November] = 30,
    [Month_December] = 31
};

u32 DaysSinceStartOfYearToBeginningOfMonth[] = {
    [Month_January] = 0,
    [Month_February] = 31,
    [Month_March] = 31 + 28,
    [Month_April] = 31 + 28 + 31,
    [Month_May] = 31 + 28 + 31 + 30,
    [Month_June] = 31 + 28 + 31 + 30 + 31,
    [Month_July] = 31 + 28 + 31 + 30 + 31 + 30,
    [Month_August] = 31 + 28 + 31 + 30 + 31 + 30 + 31,
    [Month_September] = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    [Month_October] = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    [Month_November] = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    [Month_December] = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
};

u32 DaysSinceStartOfYearToBeginningOfMonthLeapYear[] = {
    [Month_January] = 0,
    [Month_February] = 31,
    [Month_March] = 31 + 29,
    [Month_April] = 31 + 29 + 31,
    [Month_May] = 31 + 29 + 31 + 30,
    [Month_June] = 31 + 29 + 31 + 30 + 31,
    [Month_July] = 31 + 29 + 31 + 30 + 31 + 30,
    [Month_August] = 31 + 29 + 31 + 30 + 31 + 30 + 31,
    [Month_September] = 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
    [Month_October] = 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    [Month_November] = 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    [Month_December] = 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
};

typedef struct date {
    u32 Month;
    u32 Day;
    i32 Year;
} date;

typedef struct datetime {
    date Date;
    u32 Hours;
    u32 Minutes;
    u32 Seconds;
} datetime;

#define DAYS_IN_REGULAR_YEAR 365

// 4 * 365 = 1460 (and then +1 for leap year)
#define DAYS_IN_QUADYEAR 1461

// 1461 * 25 = 36525 (and then -1 for skipped leap year)
#define DAYS_IN_CENTURY 36524

// 36524 * 4 = 146096 (and then +1 for unskipped leap year)
#define DAYS_IN_QUADCENTURY 146097

// Unix epoch is January 1st, 1970
#define DAYS_SINCE_JAN1_1AD_TO_UNIX_EPOCH (DAYS_IN_QUADCENTURY * 4 + DAYS_IN_CENTURY * 3 + 17 * DAYS_IN_QUADYEAR + 2 * DAYS_IN_REGULAR_YEAR)

bool32 IsLeapYear(u32 Year);
datetime DatetimeFromUnixTimeSec(i64 UnixTimeSec);

#endif