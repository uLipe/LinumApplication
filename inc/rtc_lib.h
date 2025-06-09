#ifndef RTC_LIB
#define RTC_LIB

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>

#define DEFAULT_TD_ISO_8601_TYPE_1 "%Y-%m-%d %H:%M:%S"  // YYYY-MM-DD HH:MM:SS
#define DEFAULT_TD_ISO_8601_TYPE_2 "%d-%m-%Y %H:%M:%S"  // DD-MM-YYYY HH:MM:SS
#define DEFAULT_TD_ISO_8601_TYPE_3 "%H:%M:%S %Y-%m-%d"  // HH:MM:SS YYYY-MM-DD
#define DEFAULT_TD_ISO_8601_TYPE_4 "%H:%M:%S %d-%m-%Y"  // HH:MM:SS DD-MM-YYYY

#define DEFAULT_TD_DD_MM_YYYY_TYPE_1 "%d/%m/%Y %H:%M:%S"  // DD/MM/YYYY HH:MM:SS
#define DEFAULT_TD_YYYY_MM_DD_TYPE_2 "%Y/%m/%d %H:%M:%S"  // YYYY/MM/DD HH:MM:SS
#define DEFAULT_TD_DD_MM_YYYY_TYPE_3 "%H:%M:%S %d/%m/%Y"  // HH:MM:SS DD/MM/YYYY
#define DEFAULT_TD_YYYY_MM_DD_TYPE_4 "%H:%M:%S %Y/%m/%d"  // HH:MM:SS YYYY/MM/DD

#define DEFAULT_TIME_ISO_8601_TYPE_1 "%H:%M:%S"  // HH:MM:SS
#define DEFAULT_TIME_ISO_8601_TYPE_2 "%H:%M"     // HH:MM
#define DEFAULT_DATE_ISO_8601_TYPE_1 "%Y-%m-%d"  // YYYY-MM-DD
#define DEFAULT_DATE_ISO_8601_TYPE_2 "%d-%m-%Y"  // DD-MM-YYYY

#define DEFAULT_DD_MM_YYYY_TYPE_1 "%d/%m/%Y "  // DD/MM/YYYY
#define DEFAULT_YYYY_MM_DD_TYPE_2 "%Y/%m/%d "  // YYYY/MM/DD

typedef union {
  struct {
    uint32_t day : 8;    // 8 bits for day (0-255)
    uint32_t month : 8;  // 8 bits for month (0-255)
    uint32_t year : 16;  // 16 bits for year (0-65535)
  };
  uint32_t value;
} Date_t;

typedef union {
  struct {
    uint32_t second : 8;
    uint32_t minute : 8;
    uint32_t hour : 8;
    uint32_t millisecond : 8;
  };
  uint32_t value;
} Time_t;

Date_t getDate(void);
Time_t getTime(void);

int rtc_init(void);
int rtc_setDay(uint16_t day);
int rtc_setMonth(uint16_t month);
int rtc_setYear(uint16_t year);
int rtc_setHour(uint16_t hour);
int rtc_setMinute(uint16_t minute);
int rtc_setSecond(uint16_t second);

int rtc_set(struct rtc_time rtc_time);
int rtc_get(struct rtc_time *rtc_time);
int rtc_get_timestamp(uint32_t *timestamp);
int rtc_set_timestamp(uint32_t timestamp);
int rtc_convert_timestamp_to_rtctime(uint32_t t, struct rtc_time *rtctime);

char *rtc_format_datetime(const char *format);
char *rtc_format_date(const char *format);
char *rtc_format_time(const char *format);

bool rtc_is_leap_year(uint16_t year);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif  // RTC_LIB_H
