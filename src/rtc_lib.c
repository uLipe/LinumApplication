
#include "rtc_lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE_BUFFER_FORMAT 25

static const struct device *g_rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));

static int rtc_calc_week_day(struct rtc_time *rtctime);
static uint32_t rtc_calc_timestamp(struct rtc_time *rtctime);

static int rtc_calc_week_day(struct rtc_time *rtctime)
{
  int ret;
  uint16_t weekday_calc;
  int16_t adjustment, mm, yy;

  if (rtctime->tm_year < 2000)
  {
    rtctime->tm_year += 2000;
  }

  adjustment = (14 - rtctime->tm_mon) / 12;
  mm = rtctime->tm_mon + 12 * adjustment - 2;
  yy = rtctime->tm_year - adjustment;

  rtctime->tm_mday = rtctime->tm_mday - 1;
  weekday_calc = (rtctime->tm_mday + (13 * mm - 1) / 5 + yy + yy / 4 - yy / 100 + yy / 400) % 7;
  rtctime->tm_mday++;

  if (weekday_calc < 8)
  {
    rtctime->tm_wday = (weekday_calc + 1) % 7;

    /* If all is as expected returns a true */
    ret = 0;
  }
  else
  {
    ret = -EINVAL;
  }
  return ret;
}

uint32_t rtc_calc_timestamp(struct rtc_time *rtctime)
{
  uint16_t y;
  uint16_t m;
  uint16_t d;
  uint32_t t;

  // Year
  y = rtctime->tm_year;
  // Month of year
  m = rtctime->tm_mon;
  // Day of month
  d = rtctime->tm_mday;

  // January and February are counted as months 13 and 14 of the previous year
  if (m <= 2)
  {
    m += 12;
    y -= 1;
  }

  /* Convert years to days */
  t = (365 * y) + (y / 4) - (y / 100) + (y / 400);

  /* Convert months to days */
  t += (30 * m) + (3 * (m + 1) / 5) + d;

  /* Unix time starts on January 1st, 1970 */
  t -= 719561;

  /* Convert days to seconds */
  t *= 86400;

  /* Add hours, minutes and seconds */
  t += (3600 * rtctime->tm_hour) + (60 * rtctime->tm_min) + rtctime->tm_sec;

  // Return Unix time
  return t;
}

int rtc_init(void)
{
  int ret;
  struct rtc_time rtctime;

  ret = device_is_ready(g_rtc);
  if (!ret)
  {
    printk("Init RTC fail\n");
  }

  ret = rtc_get(&rtctime);
  if (ret)
  {
    rtctime.tm_year = 2024;
    rtctime.tm_mon = 1;
    rtctime.tm_mday = 1;
    rtctime.tm_hour = 23;
    rtctime.tm_min = 50;
    rtctime.tm_sec = 0;
    rtc_set(rtctime);
    return 0;
  }

  return ret;
}

int rtc_get(struct rtc_time *time_rtc)
{
  int ret;
  ret = rtc_get_time(g_rtc, time_rtc);
  if (!ret)
  {
    time_rtc->tm_year += 1900;
    time_rtc->tm_mon += 1;
    ret = rtc_calc_week_day(time_rtc);
  }
  return ret;
}

int rtc_get_timestamp(uint32_t *timestamp)
{
  int ret;
  struct rtc_time time_rtc;

  ret = rtc_get_time(g_rtc, &time_rtc);
  if (ret)
  {
    return ret;
  }
  time_rtc.tm_year += 1900;
  time_rtc.tm_mon += 1;
  ret = rtc_calc_week_day(&time_rtc);
  if (ret)
  {
    return ret;
  }

  *timestamp = rtc_calc_timestamp(&time_rtc);
  return 0;
}

int rtc_set_timestamp(uint32_t timestamp)
{
  int ret;
  struct rtc_time time_rtc;

  ret = rtc_convert_timestamp_to_rtctime(timestamp, &time_rtc);

  if (ret)
  {
    return ret;
  }

  rtc_set(time_rtc);

  return 0;
}

int rtc_convert_timestamp_to_rtctime(uint32_t t, struct rtc_time *rtctime)
{
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t e;
  uint32_t f;
  int err;

  // Negative Unix time values are not supported
  if (t < 1)
  {
    return -EINVAL;
  }

  memset(rtctime, 0, sizeof(struct rtc_time));

  // Retrieve hours, minutes and seconds
  rtctime->tm_sec = t % 60;
  t /= 60;
  rtctime->tm_min = t % 60;
  t /= 60;
  rtctime->tm_hour = t % 24;
  t /= 24;

  // Convert Unix time to date
  a = (uint32_t)((4 * t + 102032) / 146097 + 15);
  b = (uint32_t)(t + 2442113 + a - (a / 4));
  c = (20 * b - 2442) / 7305;
  d = b - 365 * c - (c / 4);
  e = d * 1000 / 30601;
  f = d - e * 30 - e * 601 / 1000;

  // January and February are counted as months 13 and 14 of the previous year
  if (e <= 13)
  {
    c -= 4716;
    e -= 1;
  }
  else
  {
    c -= 4715;
    e -= 13;
  }

  // Retrieve year, month and day
  rtctime->tm_year = c;
  rtctime->tm_mon = e;
  rtctime->tm_mday = f;

  // Calculate day of week
  err = rtc_calc_week_day(rtctime);

  return err;
}

int rtc_set(struct rtc_time time_rtc)
{
  int ret;
  struct rtc_time time = {0};

  time.tm_hour = time_rtc.tm_hour;
  time.tm_min = time_rtc.tm_min;
  time.tm_sec = time_rtc.tm_sec;
  time.tm_mday = time_rtc.tm_mday;
  time.tm_mon = time_rtc.tm_mon;
  time.tm_year = time_rtc.tm_year;

  /* Invalid date */
  if (time_rtc.tm_year <= 2020)
  {
    return -EINVAL;
  }

  time.tm_year -= 1900;

  if (time.tm_mon)
  {
    time.tm_mon -= 1;
  }

  ret = rtc_set_time(g_rtc, &time);
  return ret;
}

char *rtc_format_datetime(const char *format)
{
  static char buffer[SIZE_BUFFER_FORMAT];
  struct tm timeInfo;
  struct rtc_time rtc_time;

  rtc_get(&rtc_time);

  timeInfo.tm_year = rtc_time.tm_year;
  timeInfo.tm_mon = rtc_time.tm_mon;
  timeInfo.tm_mday = rtc_time.tm_mday;
  timeInfo.tm_hour = rtc_time.tm_hour;
  timeInfo.tm_min = rtc_time.tm_min;
  timeInfo.tm_sec = rtc_time.tm_sec;

  if (timeInfo.tm_year)
  {
    timeInfo.tm_year -= 1900;
  }

  if (timeInfo.tm_mon)
  {
    timeInfo.tm_mon -= 1;
  }

  if (format == NULL)
  {
    strftime(buffer, sizeof(buffer), DEFAULT_TD_DD_MM_YYYY_TYPE_3, &timeInfo);
  }
  else
  {
    strftime(buffer, sizeof(buffer), format, &timeInfo);
  }

  return buffer;
}

char *rtc_format_date(const char *format)
{
  static char buffer[SIZE_BUFFER_FORMAT];
  struct tm timeInfo;
  struct rtc_time rtc_time;

  rtc_get(&rtc_time);

  timeInfo.tm_year = rtc_time.tm_year;
  timeInfo.tm_mon = rtc_time.tm_mon;
  timeInfo.tm_mday = rtc_time.tm_mday;

  if (timeInfo.tm_year)
  {
    timeInfo.tm_year -= 1900;
  }

  if (timeInfo.tm_mon)
  {
    timeInfo.tm_mon -= 1;
  }

  if (format != NULL)
  {
    strftime(buffer, sizeof(buffer), format, &timeInfo);
  }
  else
  {
    strftime(buffer, sizeof(buffer), DEFAULT_DATE_ISO_8601_TYPE_2, &timeInfo);
  }

  return buffer;
}

char *rtc_format_time(const char *format)
{
  static char buffer[SIZE_BUFFER_FORMAT];
  struct tm timeInfo;
  struct rtc_time rtc_time;

  rtc_get(&rtc_time);

  timeInfo.tm_year = rtc_time.tm_year;
  timeInfo.tm_mon = rtc_time.tm_mon;
  timeInfo.tm_mday = rtc_time.tm_mday;

  timeInfo.tm_hour = rtc_time.tm_hour;
  timeInfo.tm_min = rtc_time.tm_min;
  timeInfo.tm_sec = rtc_time.tm_sec;

  if (format != NULL)
  {
    strftime(buffer, sizeof(buffer), format, &timeInfo);
  }
  else
  {
    strftime(buffer, sizeof(buffer), DEFAULT_TIME_ISO_8601_TYPE_1, &timeInfo);
  }
  return buffer;
}

bool rtc_is_leap_year(uint16_t year)
{
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
  {
    return true; // É bissexto
  }
  else
  {
    return false; // Não é bissexto
  }
}
