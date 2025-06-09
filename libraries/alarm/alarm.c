#include "alarm.h"

#include <stdbool.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "rtc_lib.h"

static int alarm_mutex_lock(struct alarm *alarm);
static int alarm_mutex_unlock(struct alarm *alarm);
static int alarm_memory_add(struct alarm *alarm, int id, uint32_t timestamp);
static int alarm_set(struct alarm *alarm, int id);
static int alarm_clear(struct alarm *alarm, int id);
static uint32_t get_current_timestamp(void);
static void hysteresis_timer_callback(struct k_timer *timer);
static int alarm_event_add(struct alarm *alarm, int id, bool state, uint32_t timestamp);
static void alarm_change_work_handler(struct k_work *work);
static void alarm_mem_clear_work_handler(struct k_work *work);

static int alarm_mutex_lock(struct alarm *alarm) {
  return k_mutex_lock(&alarm->mutex, K_FOREVER);
}

static int alarm_mutex_unlock(struct alarm *alarm) {
  return k_mutex_unlock(&alarm->mutex);
}

static int alarm_event_add(struct alarm *alarm, int id, bool state, uint32_t timestamp) {
  alarm_event_t event;
  int ret;

  if (!alarm || id >= ALARM_COUNT || !alarm->descriptions || 
      id >= alarm->description_count) {
    return -EINVAL;
  }

  event.timestamp = timestamp;
  event.alarm_id = id;
  event.state = state;
  event.alarm_mask = alarm->actived.mask;
  event.mem_alarm_mask = alarm->memory.mask;
  event.severity = alarm->descriptions[id].severity;
  snprintf(event.description, sizeof(event.description), "%s", alarm->descriptions[id].message);

  ret = k_msgq_put(&alarm->event_queue, &event, K_NO_WAIT);
  if (ret == 0) {
    k_work_submit(&alarm->alarm_change_work);
  }

  return ret;
}

static void alarm_change_work_handler(struct k_work *work) {
  struct alarm *alarm = CONTAINER_OF(work, struct alarm, alarm_change_work);
  alarm_event_t event;
  
  while (k_msgq_get(&alarm->event_queue, &event, K_NO_WAIT) == 0) {
    if (alarm->event_callback) {
      alarm->event_callback(&event);
    }
  }
}

// Handler do work de limpar memória
static void alarm_mem_clear_work_handler(struct k_work *work) {
  struct alarm *alarm = CONTAINER_OF(work, struct alarm, mem_clear_work);
  
  // Chamar o callback de aplicação
  if (alarm->mem_clear_callback) {
    alarm->mem_clear_callback();
  }
}

static int alarm_memory_add(struct alarm *alarm, int id, uint32_t timestamp) {
  bool already_set;
  int err = -EINVAL;

  if (id < ALARM_COUNT) {
    err = alarm_mutex_lock(alarm);
    if (err) {
      return err;
    }

    already_set = (alarm->memory.mask & (1U << id));
    alarm->memory.mask |= (1U << id);

    if (!already_set) {
      alarm->memory.on_timestamp[id] = timestamp;
    }

    alarm_mutex_unlock(alarm);
    err = 0;
  }

  return err;
}

static int alarm_set(struct alarm *alarm, int id) {
  bool was_set;
  int err = -EINVAL;

  if (id < ALARM_COUNT) {
    err = alarm_mutex_lock(alarm);
    if (err) {
      return err;
    }

    was_set = (alarm->actived.mask & (1U << id));
    alarm->actived.mask |= (1U << id);
    alarm_mutex_unlock(alarm);

    if (!was_set) {
      uint32_t current_time = get_current_timestamp();
      alarm->actived.on_timestamp[id] = current_time;
      err = alarm_memory_add(alarm, id, current_time);
    }
  }

  return err;
}

static int alarm_clear(struct alarm *alarm, int id) {
  bool was_set;
  int err = -EINVAL;

  if (id < ALARM_COUNT) {
    err = alarm_mutex_lock(alarm);
    if (err) {
      return err;
    }

    was_set = (alarm->actived.mask & (1U << id));
    alarm->actived.mask &= ~(1U << id);

    alarm_mutex_unlock(alarm);

    if (was_set) {
      alarm->actived.off_timestamp[id] = get_current_timestamp();
    }
  }

  return err;
}

static uint32_t get_current_timestamp(void) {
  uint32_t timestamp;
  rtc_get_timestamp(&timestamp);
  return timestamp;
}

static void hysteresis_timer_callback(struct k_timer *timer) {
  uint32_t timestamp;
  struct alarm_timer_data *data = (struct alarm_timer_data *)timer->user_data;
  struct alarm *alarm = (struct alarm *)data->alarm;

  if (data) {
    if (data->state) {
      alarm_set(alarm, data->id);
      timestamp = alarm->actived.on_timestamp[data->id];
    } else {
      alarm_clear(alarm, data->id);
      timestamp = alarm->actived.off_timestamp[data->id];
    }

    if (alarm->event_callback && alarm->descriptions && 
        data->id < alarm->description_count) {
      alarm_event_add(alarm, data->id, data->state, timestamp);
    }
  }
}

int alarm_init(struct alarm *alarm, const struct alarm_list *descriptions, uint16_t description_count,
               alarm_event_cb_t event_callback, alarm_mem_clear_cb_t mem_clear_callback,
               uint16_t queue_size) {
  
  if (alarm == NULL) {
    return -EINVAL;
  }
  
  if (queue_size == 0) {
    queue_size = 1;
  } else if (queue_size > ALARM_EVENT_QUEUE_SIZE) {
    queue_size = ALARM_EVENT_QUEUE_SIZE;
  }
  
  memset(alarm, 0, sizeof(struct alarm));
  
  k_mutex_init(&alarm->mutex);
  
  k_msgq_init(&alarm->event_queue, 
              alarm->event_msgq_buffer,
              sizeof(alarm_event_t), 
              queue_size);
  
  for (int i = 0; i < ALARM_COUNT; i++) {
    alarm->actived.hysteresis[i].msec = 0;
    alarm->actived.hysteresis[i].pending.state = false;
    alarm->actived.hysteresis[i].pending.id = -1;
    alarm->actived.hysteresis[i].pending.alarm = alarm;
    k_timer_init(&alarm->actived.hysteresis[i].timer, hysteresis_timer_callback, NULL);
  }
  
  // Armazenar callbacks
  alarm->event_callback = event_callback;
  alarm->mem_clear_callback = mem_clear_callback;
  
  // Inicializar work queues
  k_work_init(&alarm->alarm_change_work, alarm_change_work_handler);
  k_work_init(&alarm->mem_clear_work, alarm_mem_clear_work_handler);
  
  // Configurar descrições
  if (descriptions && description_count) {
    alarm->descriptions = descriptions;
    alarm->description_count = description_count;
  }
  
  return 0;
}

// int alarm_init_default(struct alarm *alarm, const struct alarm_list *descriptions, uint16_t description_count,
//                       alarm_event_cb_t event_callback, alarm_mem_clear_cb_t mem_clear_callback) {
//   return alarm_init(alarm, descriptions, description_count, event_callback, mem_clear_callback,
//                    ALARM_EVENT_QUEUE_SIZE);
// }

int alarm_deinit(struct alarm *alarm) {
  if (alarm == NULL) {
    return -EINVAL;
  }
  
  for (int i = 0; i < ALARM_COUNT; i++) {
    k_timer_stop(&alarm->actived.hysteresis[i].timer);
  }
  
  k_msgq_purge(&alarm->event_queue);
  
  return 0;
}

// Limpar a fila de eventos
int alarm_flush_event_queue(struct alarm *alarm) {
  if (alarm == NULL) {
    return -EINVAL;
  }
  
  k_msgq_purge(&alarm->event_queue);
  
  return 0;
}

int alarm_set_status(struct alarm *alarm, int id, bool status) {
  int err = -EINVAL;
  
  if (alarm == NULL || id >= ALARM_COUNT) {
    return err;
  }
  
  if (status == alarm_is_set(alarm, id)) {
    k_timer_stop(&alarm->actived.hysteresis[id].timer);
    alarm->actived.hysteresis[id].pending.id = -1;
    return 0;
  }
  
  if (alarm->actived.hysteresis[id].msec == 0) {
    if (status) {
      err = alarm_set(alarm, id);
      
      if (err == 0 && alarm->event_callback) {
        uint32_t timestamp = alarm->actived.on_timestamp[id];
        alarm_event_add(alarm, id, status, timestamp);
      }
    } else {
      err = alarm_clear(alarm, id);
      
      if (err == 0 && alarm->event_callback) {
        uint32_t timestamp = alarm->actived.off_timestamp[id];
        alarm_event_add(alarm, id, status, timestamp);
      }
    }
    return err;
  }
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  alarm->actived.hysteresis[id].pending.id = id;
  alarm->actived.hysteresis[id].pending.state = status;
  alarm->actived.hysteresis[id].pending.alarm = alarm;
  alarm_mutex_unlock(alarm);
  
  k_timer_user_data_set(&alarm->actived.hysteresis[id].timer,
                        (void *)&alarm->actived.hysteresis[id].pending);
  k_timer_start(&alarm->actived.hysteresis[id].timer,
                K_MSEC(alarm->actived.hysteresis[id].msec), K_NO_WAIT);
  
  return 0;
}

int alarm_force_set(struct alarm *alarm, uint32_t status) {
  int err = -EINVAL;
  bool is_set;
  
  if (alarm == NULL || alarm->descriptions == NULL) {
    return err;
  }
  
  for (int i = 0; i < alarm->description_count; i++) {
    is_set = (status & (1U << i)) ? true : false;
    if (is_set) {
      err = alarm_set(alarm, i);
      
      if (err == 0 && alarm->event_callback) {
        uint32_t timestamp = alarm->actived.on_timestamp[i];
        alarm_event_add(alarm, i, true, timestamp);
      }
    } else {
      err = alarm_clear(alarm, i);
      
      if (err == 0 && alarm->event_callback) {
        uint32_t timestamp = alarm->actived.off_timestamp[i];
        alarm_event_add(alarm, i, false, timestamp);
      }
    }
  }
  
  return err;
}

int alarm_set_hysteresis(struct alarm *alarm, int id, uint32_t hysteresis_ms) {
  int err = -EINVAL;
  
  if (alarm == NULL || id >= ALARM_COUNT) {
    return err;
  }
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  alarm->actived.hysteresis[id].msec = hysteresis_ms;
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarm_get_hysteresis(struct alarm *alarm, int id, uint32_t *hysteresis_ms) {
  int err = -EINVAL;
  
  if (alarm == NULL || id >= ALARM_COUNT || hysteresis_ms == NULL) {
    return err;
  }
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  *hysteresis_ms = alarm->actived.hysteresis[id].msec;
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarm_get_last_on_timestamp(struct alarm *alarm, int id, uint32_t *timestamp) {
  int err = -EINVAL;
  
  if (alarm == NULL || id >= ALARM_COUNT || timestamp == NULL) {
    return err;
  }
  
  *timestamp = 0;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  *timestamp = alarm->actived.on_timestamp[id];
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarm_get_last_off_timestamp(struct alarm *alarm, int id, uint32_t *timestamp) {
  int err = -EINVAL;
  
  if (alarm == NULL || id >= ALARM_COUNT || timestamp == NULL) {
    return err;
  }
  
  *timestamp = 0;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  *timestamp = alarm->actived.off_timestamp[id];
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarm_memory_get_timestamp(struct alarm *alarm, int id, uint32_t *timestamp) {
  int err = -EINVAL;
  
  if (alarm == NULL || id >= ALARM_COUNT || timestamp == NULL) {
    return err;
  }
  
  *timestamp = 0;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  *timestamp = alarm->memory.on_timestamp[id];
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarms_get_descrition(struct alarm *alarm, int alarm_id, char *buf, int buflen) {
  int i;
  int ret = -1;
  
  if (alarm == NULL || buf == NULL || alarm->descriptions == NULL || buflen <= 0) {
    return -EINVAL;
  }
  
  for (i = 0; i < alarm->description_count; i++) {
    if (alarm->descriptions[i].id == alarm_id) {
      snprintf(buf, buflen, "%s", alarm->descriptions[i].message);
      ret = 0;
      break;
    }
  }
  
  if (ret) {
    snprintf(buf, buflen, "Alarm %d not found", alarm_id);
  }
  
  return ret;
}

int alarms_get_severety_string(struct alarm *alarm, int severety_id, char *buf, int buflen) {
  if (alarm == NULL || buf == NULL || buflen <= 0) {
    return -EINVAL;
  }
  
  switch (severety_id) {
    case SEVERITY_INFO:
      snprintf(buf, buflen, "INFO");
      break;
    case SEVERITY_WARNING:
      snprintf(buf, buflen, "WARNING");
      break;
    case SEVERITY_ERROR:
      snprintf(buf, buflen, "ERROR");
      break;
    case SEVERITY_CRITICAL:
      snprintf(buf, buflen, "CRITICAL");
      break;
    default:
      snprintf(buf, buflen, "UNKNOWN");
      return -EINVAL;
  };
  return 0;
}

int alarms_get_severety(struct alarm *alarm, int alarm_id, int *severety) {
  int i;
  int ret = -EINVAL;
  
  if (alarm == NULL || alarm->descriptions == NULL || severety == NULL) {
    return -EINVAL;
  }
  
  for (i = 0; i < alarm->description_count; i++) {
    if (alarm->descriptions[i].id == alarm_id) {
      *severety = alarm->descriptions[i].severity;
      ret = 0;
      break;
    }
  }
  
  return ret;
}

bool alarm_is_set(struct alarm *alarm, int id) {
  int err;
  bool result = false;
  
  if (alarm == NULL || id >= ALARM_COUNT) {
    return false;
  }
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return false;
  }
  
  result = (alarm->actived.mask & (1U << id)) ? true : false;
  alarm_mutex_unlock(alarm);
  
  return result;
}

bool alarm_memory_is_set(struct alarm *alarm, int id) {
  int err;
  bool result = false;
  
  if (alarm == NULL || id >= ALARM_COUNT) {
    return false;
  }
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return false;
  }
  
  result = (alarm->memory.mask & (1U << id)) ? true : false;
  alarm_mutex_unlock(alarm);
  
  return result;
}

int alarm_get_status(struct alarm *alarm, uint32_t *mask) {
  int err = -EINVAL;
  
  if (alarm == NULL || mask == NULL) {
    return err;
  }
  
  *mask = 0;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  *mask = alarm->actived.mask;
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarm_get_mem_status(struct alarm *alarm, uint32_t *mask) {
  int err = -EINVAL;
  
  if (alarm == NULL || mask == NULL) {
    return err;
  }
  
  *mask = 0;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  *mask = alarm->memory.mask;
  alarm_mutex_unlock(alarm);
  
  return 0;
}

int alarm_memory_clear(struct alarm *alarm) {
  int err = -EINVAL;
  uint32_t active_mask;
  uint32_t temp_timestamps[ALARM_COUNT];
  
  if (alarm == NULL) {
    return err;
  }
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    return err;
  }
  
  active_mask = alarm->actived.mask;
  memcpy(temp_timestamps, alarm->memory.on_timestamp, sizeof(temp_timestamps));
  alarm->memory.mask = 0;
  
  for (int i = 0; i < ALARM_COUNT; i++) {
    if (active_mask & (1U << i)) {
      alarm->memory.mask |= (1U << i);
      alarm->memory.on_timestamp[i] = temp_timestamps[i];
    } else {
      alarm->memory.on_timestamp[i] = 0;
    }
  }
  
  alarm_mutex_unlock(alarm);
  
  // Submeter o work para o callback de limpeza de memória
  k_work_submit(&alarm->mem_clear_work);
  
  return 0;
}

int alarms_show_alarms_list(struct alarm *alarm) {
  int err = -EINVAL;
  severity_t severity;
  const char *severity_str;
  const char *message = "No description";
  uint32_t hysteresis;
  
  if (alarm == NULL || alarm->descriptions == NULL) {
    return err;
  }
  
  printk("\nAlarm list:\n");
  printk("----------------\n");
  
  for (int i = 0; i < alarm->description_count; i++) {
    severity = SEVERITY_INFO;
    
    for (int j = 0; j < alarm->description_count; j++) {
      if (alarm->descriptions[j].id == i) {
        message = alarm->descriptions[j].message;
        severity = alarm->descriptions[j].severity;
        break;
      }
    }
    
    switch (severity) {
      case SEVERITY_INFO:
        severity_str = "INFO";
        break;
      case SEVERITY_WARNING:
        severity_str = "WARNING";
        break;
      case SEVERITY_ERROR:
        severity_str = "ERROR";
        break;
      case SEVERITY_CRITICAL:
        severity_str = "CRITICAL";
        break;
      default:
        severity_str = "UNKNOWN";
        break;
    }
    
    printk("ID: %-3d | Severity: %-8s | Description: %s\n", i, severity_str, message);
    
    alarm_get_hysteresis(alarm, i, &hysteresis);
    if (hysteresis > 0) {
      printk("\t| Hysteresis: %u ms\n", hysteresis);
    }
  }
  
  printk("----------------");
  
  return 0;
}

int alarms_show_alarms_actived(struct alarm *alarm) {
  int err = -EINVAL;
  
  if (alarm == NULL || alarm->descriptions == NULL) {
    return err;
  }
  
  printk("Active alarms:\n");
  printk("----------------\n");
  
  bool has_active_alarms = false;
  const char *severity_str;
  uint32_t active_alarms;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    printk("Error accessing alarm data\n");
    return err;
  }
  
  active_alarms = alarm->actived.mask;
  alarm_mutex_unlock(alarm);
  
  for (int i = 0; i < ALARM_COUNT; i++) {
    if (active_alarms & (1U << i)) {
      has_active_alarms = true;
      
      const char *message = "No description";
      severity_t severity = SEVERITY_INFO;
      uint32_t on_timestamp, off_timestamp, hysteresis_ms;
      
      for (int j = 0; j < alarm->description_count; j++) {
        if (alarm->descriptions[j].id == i) {
          message = alarm->descriptions[j].message;
          severity = alarm->descriptions[j].severity;
          break;
        }
      }
      
      err = alarm_mutex_lock(alarm);
      if (err) {
        printk("Error accessing alarm data\n");
        return err;
      }
      
      on_timestamp = alarm->actived.on_timestamp[i];
      off_timestamp = alarm->actived.off_timestamp[i];
      hysteresis_ms = alarm->actived.hysteresis[i].msec;
      
      alarm_mutex_unlock(alarm);
      
      switch (severity) {
        case SEVERITY_INFO:
          severity_str = "INFO";
          break;
        case SEVERITY_WARNING:
          severity_str = "WARNING";
          break;
        case SEVERITY_ERROR:
          severity_str = "ERROR";
          break;
        case SEVERITY_CRITICAL:
          severity_str = "CRITICAL";
          break;
        default:
          severity_str = "UNKNOWN";
          break;
      }
      
      printk("ID: %-3d | Severity: %-8s | Description: %s\n", i, severity_str, message);
      printk("\t| Last ON: %u | Last OFF: %u\n", on_timestamp, off_timestamp);
      printk("\t| Hysteresis: %u ms\n", hysteresis_ms);
    }
  }
  
  if (!has_active_alarms) {
    printk("No active alarms at the moment.\n");
  }
  printk("----------------\n");
  
  return 0;
}

int alarm_show_alarms_memory(struct alarm *alarm) {
  int err = -EINVAL;
  
  if (alarm == NULL || alarm->descriptions == NULL) {
    return err;
  }
  
  printk("Alarm memory (all alarms that occurred):\n");
  printk("---------------------------------------\n");
  
  bool has_alarms_in_memory = false;
  const char *severity_str;
  uint32_t memory_alarms;
  
  err = alarm_mutex_lock(alarm);
  if (err) {
    printk("Error accessing alarm data\n");
    return err;
  }
  
  memory_alarms = alarm->memory.mask;
  alarm_mutex_unlock(alarm);
  
  for (int i = 0; i < ALARM_COUNT; i++) {
    if (memory_alarms & (1U << i)) {
      has_alarms_in_memory = true;
      
      const char *message = "No description";
      severity_t severity = SEVERITY_INFO;
      uint32_t on_timestamp, off_timestamp, on_memory_timestamp, hysteresis_ms;
      
      for (int j = 0; j < alarm->description_count; j++) {
        if (alarm->descriptions[j].id == i) {
          message = alarm->descriptions[j].message;
          severity = alarm->descriptions[j].severity;
          break;
        }
      }
      
      err = alarm_mutex_lock(alarm);
      if (err) {
        printk("Error accessing alarm data\n");
        return err;
      }
      
      on_timestamp = alarm->actived.on_timestamp[i];
      off_timestamp = alarm->actived.off_timestamp[i];
      on_memory_timestamp = alarm->memory.on_timestamp[i];
      hysteresis_ms = alarm->actived.hysteresis[i].msec;
      
      alarm_mutex_unlock(alarm);
      
      switch (severity) {
        case SEVERITY_INFO:
          severity_str = "INFO";
          break;
        case SEVERITY_WARNING:
          severity_str = "WARNING";
          break;
        case SEVERITY_ERROR:
          severity_str = "ERROR";
          break;
        case SEVERITY_CRITICAL:
          severity_str = "CRITICAL";
          break;
        default:
          severity_str = "UNKNOWN";
          break;
      }
      
      printk("ID: %-3d | Severity: %-8s | Description: %s\n", i, severity_str, message);
      printk("\t| Last ON: %u | Last OFF: %u\n", on_timestamp, off_timestamp);
      printk("\t| Memory timestamp: %u\n", on_memory_timestamp);
      printk("\t| Hysteresis: %u ms\n", hysteresis_ms);
    }
  }
  
  if (!has_alarms_in_memory) {
    printk("No alarms in memory.\n");
  }
  printk("---------------------------------------\n");
  
  return 0;
}