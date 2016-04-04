/*
 * MyTime.h
 *
 *  Created on: 3 Apr 2016
 *      Author: Willem Eradus
 */


#ifndef WETIME_H_
#define WETIME_H_

#include <stdint.h>
#include <time.h>

#define SECSPERMIN  60L
#define MINSPERHOUR 60L
#define HOURSPERDAY 24L
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY  (SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK 7
#define MONSPERYEAR 12

#define YEAR_BASE   1900
#define EPOCH_YEAR  1970
#define EPOCH_WDAY  4
#define EPOCH_YEARS_SINCE_LEAP 2
#define EPOCH_YEARS_SINCE_CENTURY 70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#define EPOCH_ADJUSTMENT_DAYS   719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR 0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY 3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA        146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY    36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS    (3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR       365
/* number of days in January */
#define DAYS_IN_JANUARY     31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY    28
/* number of years per era */
#define YEARS_PER_ERA       400

class Time {
public:
    Time();
    static bool dst(uint32_t lcltime) ;
    static void localtime (const uint32_t utctime,struct tm *t) ;

private:
    static inline int LastSunday(int dow,int dom,int max);
    static bool daylightsavings(int hour,int day, int month, int dow);

};

#endif /* WETIME_H_ */
