#ifndef _ALARM_H
#define _ALARM_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/task_wdt/task_wdt.h>

#define ALARM_COUNT              (32)
#define ALARM_LIST_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define ALARM_EVENT_QUEUE_SIZE   (10)

typedef enum {
  SEVERITY_INFO,
  SEVERITY_WARNING,
  SEVERITY_ERROR,
  SEVERITY_CRITICAL,
  SEVERITY_MAX,
} severity_t;

struct alarm_list {
  int id;
  severity_t severity;
  const char *message;
};

typedef struct {
  bool state; 
  int alarm_id;
  uint32_t timestamp;
  severity_t severity;
  uint32_t alarm_mask;
  uint32_t mem_alarm_mask;
  char description[30];
} alarm_event_t;

typedef void (*alarm_event_cb_t)(const alarm_event_t *event);
typedef void (*alarm_mem_clear_cb_t)(void);

struct alarm_timer_data {
  int id;
  bool state;
  void *alarm;
};

struct alarm_hysteresis {
  uint32_t msec;
  struct alarm_timer_data pending;
  struct k_timer timer;
};

struct alarm_actived {
  uint32_t mask;
  uint32_t on_timestamp[ALARM_COUNT];
  uint32_t off_timestamp[ALARM_COUNT];
  struct alarm_hysteresis hysteresis[ALARM_COUNT];
};

struct alarm_mem_actived {
  uint32_t mask;
  uint32_t on_timestamp[ALARM_COUNT];
};

struct alarm {
  struct alarm_actived actived;
  struct alarm_mem_actived memory;
  struct k_mutex mutex;
  struct k_work alarm_change_work;
  struct k_work mem_clear_work;
  struct k_msgq event_queue;
  char __aligned(4) event_msgq_buffer[ALARM_EVENT_QUEUE_SIZE * sizeof(alarm_event_t)];
  alarm_event_cb_t event_callback;
  alarm_mem_clear_cb_t mem_clear_callback;
  const struct alarm_list *descriptions;
  uint16_t description_count;
};

int alarm_init(struct alarm *alarm, const struct alarm_list *descriptions, uint16_t description_count, 
               alarm_event_cb_t event_callback, alarm_mem_clear_cb_t mem_clear_callback,
               uint16_t queue_size);
int alarm_set_status(struct alarm *alarm, int id, bool status);
int alarm_force_set(struct alarm *alarm, uint32_t status);
int alarm_set_hysteresis(struct alarm *alarm, int id, uint32_t hysteresis_ms);
int alarm_get_hysteresis(struct alarm *alarm, int id, uint32_t *hysteresis_ms);
int alarm_get_last_on_timestamp(struct alarm *alarm, int id, uint32_t *timestamp);
int alarm_get_last_off_timestamp(struct alarm *alarm, int id, uint32_t *timestamp);
int alarm_memory_get_timestamp(struct alarm *alarm, int id, uint32_t *timestamp);
int alarms_get_descrition(struct alarm *alarm, int alarm_id, char *buf, int buflen);
int alarms_get_severety_string(struct alarm *alarm, int severety_id, char *buf, int buflen);
int alarms_get_severety(struct alarm *alarm, int alarm_id, int *severety);
bool alarm_is_set(struct alarm *alarm, int id);
bool alarm_memory_is_set(struct alarm *alarm, int id);
int alarm_get_status(struct alarm *alarm, uint32_t *mask);
int alarm_get_mem_status(struct alarm *alarm, uint32_t *mask);
int alarm_memory_clear(struct alarm *alarm);
int alarms_show_alarms_list(struct alarm *alarm);
int alarms_show_alarms_actived(struct alarm *alarm);
int alarm_show_alarms_memory(struct alarm *alarm);

int alarm_flush_event_queue(struct alarm *alarm);

int alarm_deinit(struct alarm *alarm);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* _ALARM_H*/