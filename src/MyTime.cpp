/*
 * Time.cpp
 *
 *  Created on: 3 Apr 2016
 *      Author: willem
 */

#include "MyTime.h"

Time::Time() {
    // TODO Auto-generated constructor stub
    // Not used for static functions
}

bool Time::dst(uint32_t lcltime) {
     // *
     // * compute year, month, day & day of yea
     // * for description of this algorithm see
     // * http://howardhinnant.github.io/date_algorithms.html#civil_from_days
     // *
     int days    = lcltime / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
     int rem     = lcltime % SECSPERDAY;
     int hour    = rem / SECSPERHOUR;
     int weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK);
     //
     int era     = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
     int eraday  = days - era * DAYS_PER_ERA;    /* [0, 146096] */
     int erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY - eraday / (DAYS_PER_ERA - 1)) / 365;   /* [0, 399] */
     //
     int yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100); /* [0, 365] */
     int month   = (5 * yearday + 2) / 153;
     int day     = yearday - (153 * month + 2) / 5 + 1;  /* [1, 31] */
     month  += month < 10 ? 2 : -10;  // [0, 11]
     //
     return daylightsavings(hour,day,month+1,weekday);
 }

void Time::localtime (const uint32_t utctime,struct tm *t) {
     //
     bool isdst;

     uint32_t lcltime = utctime + 3600;  // Adjust for time zone
     if ((isdst = dst(lcltime)))
         lcltime += 3600;                // Adjust for daylight savings
     //
     int days    = lcltime / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
     int rem     = lcltime % SECSPERDAY;
     int weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK);
     //
     int era     = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
     int eraday  = days - era * DAYS_PER_ERA;    /* [0, 146096] */
     int erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY - eraday / (DAYS_PER_ERA - 1)) / 365;   /* [0, 399] */
     //
     int yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100); /* [0, 365] */
     int month   = (5 * yearday + 2) / 153;
     int day     = yearday - (153 * month + 2) / 5 + 1;  /* [1, 31] */
     month  += month < 10 ? 2 : -10;  // [0, 11]
     //
     int year    = ADJUSTED_EPOCH_YEAR + erayear + era * YEARS_PER_ERA + (month <= 1);
     t->tm_yday  = yearday >= DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY ? yearday - (DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY) : yearday + DAYS_IN_JANUARY + DAYS_IN_FEBRUARY + isleap(erayear);
     t->tm_year  = year - YEAR_BASE;
     t->tm_mon   = month;
     t->tm_mday  = day;
     t->tm_wday  = weekday;
     //
     t->tm_hour = (int) (rem / SECSPERHOUR);
     rem %= SECSPERHOUR;
     t->tm_min = (int) (rem / SECSPERMIN);
     t->tm_sec = (int) (rem % SECSPERMIN);
     //
     t->tm_isdst = isdst;
     }

inline int Time::LastSunday(int dow,int dom,int max) {
     if (dow >  0 && (dom-dow) >= max-6) return 1;   // After last Sunday
     if (dow == 0 && (dom-dow) >= max-6) return 0;   // last Sunday
     return -1;                                      // Before last Sunday
 }

 bool Time::daylightsavings(int hour,int day, int month, int dow) {
     if (month < 3 || month > 10) return false;
     if (month > 3 && month < 10) return true;
     //
     if (month == 3) {
         int i = LastSunday(dow,day,31);
         if (i > 0 || (i == 0 && hour >= 2)) return true;
     }
     //
     if (month == 10) {
         int i = LastSunday(dow,day,31);
         if (i < 0 || (i == 0 && hour < 2)) return true;
     }
     return false;
 }

