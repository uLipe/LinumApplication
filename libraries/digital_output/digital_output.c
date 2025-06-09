/**
 * @file digital_output.c
 */

#include "digital_output.h"

#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(bsp_doutput);

static int digital_output_mutex_lock(digital_output_t *valve);
static int digital_output_mutex_unlock(digital_output_t *valve);
static uint32_t get_current_timestamp(void);

static const digital_output_config_list_t *find_digital_output_config(
    digital_output_t *valve, int id);

static int digital_output_mutex_lock(digital_output_t *valve) {
  return k_mutex_lock(&valve->mutex, K_FOREVER);
}

static int digital_output_mutex_unlock(digital_output_t *valve) {
  return k_mutex_unlock(&valve->mutex);
}

static uint32_t get_current_timestamp(void) { 
  return k_uptime_get_32(); 
}

static const digital_output_config_list_t *find_digital_output_config(
    digital_output_t *valve, int id) {
  if (valve == NULL || valve->config_list == NULL) {
    return NULL;
  }

  for (int i = 0; i < valve->config_count; i++) {
    if (valve->config_list[i].id == id) {
      return &valve->config_list[i];
    }
  }

  return NULL;
}

int digital_output_init(digital_output_t *valve,
                        const digital_output_config_list_t *config_list,
                        uint16_t config_count,
                        digital_output_callback_t callback) {
  int ret;

  if (valve == NULL || config_list == NULL || config_count == 0 ||
      config_count > DOUT_MAX_COUNT) {
    return -EINVAL;
  }

  memset(valve, 0, sizeof(digital_output_t));
  k_mutex_init(&valve->mutex);

  valve->config_list = config_list;
  valve->config_count = config_count;
  valve->callback = callback;

  for (int i = 0; i < config_count; i++) {
    const struct gpio_dt_spec *gpio = config_list[i].gpio;

    if (!gpio_is_ready_dt(gpio)) {
      LOG_ERR("GPIO not ready for valve ID %d", config_list[i].id);
      return -ENODEV;
    }

    ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
      LOG_ERR("Failed to configure GPIO for valve ID %d: %d", config_list[i].id,
              ret);
      return ret;
    }

    int close_state = config_list[i].active_high ? 0 : 1;
    ret = gpio_pin_set_dt(gpio, close_state);
    if (ret < 0) {
      LOG_ERR("Failed to set initial state for valve ID %d: %d",
              config_list[i].id, ret);
      return ret;
    }
  }

  LOG_INF("Valve system initialized with %d valves", config_count);
  return 0;
}

int digital_output_open(digital_output_t *valve, int id) {
  int ret;
  bool was_open;
  uint32_t timestamp;
  const digital_output_config_list_t *config;

  if (valve == NULL) {
    return -EINVAL;
  }

  config = find_digital_output_config(valve, id);
  if (config == NULL) {
    LOG_ERR("Valve ID %d not found in configuration", id);
    return -EINVAL;
  }

  ret = digital_output_mutex_lock(valve);
  if (ret < 0) {
    return ret;
  }

  was_open = (valve->status_mask & (1U << id)) ? true : false;

  if (!was_open) {
    valve->status_mask |= (1U << id);
    digital_output_mutex_unlock(valve);

    int open_state = config->active_high ? 1 : 0;

    ret = gpio_pin_set_dt(config->gpio, open_state);
    if (ret < 0) {
      LOG_ERR("Failed to open valve ID %d: %d", id, ret);

      digital_output_mutex_lock(valve);
      valve->status_mask &= ~(1U << id);
      digital_output_mutex_unlock(valve);

      return ret;
    }

    if (valve->callback != NULL) {
      timestamp = get_current_timestamp();
      valve->callback(config, true, timestamp, valve->status_mask);
    }

    LOG_INF("Valve ID %d opened", id);
  } else {
    digital_output_mutex_unlock(valve);
    LOG_DBG("Valve ID %d already open", id);
  }

  return 0;
}

int digital_output_close(digital_output_t *valve, int id) {
  int ret;
  bool was_open;
  uint32_t timestamp;
  const digital_output_config_list_t *config;

  if (valve == NULL) {
    return -EINVAL;
  }

  config = find_digital_output_config(valve, id);
  if (config == NULL) {
    LOG_ERR("Valve ID %d not found in configuration", id);
    return -EINVAL;
  }

  ret = digital_output_mutex_lock(valve);
  if (ret < 0) {
    return ret;
  }

  was_open = (valve->status_mask & (1U << id)) ? true : false;

  if (was_open) {
    valve->status_mask &= ~(1U << id);
    digital_output_mutex_unlock(valve);

    int close_state = config->active_high ? 0 : 1;

    ret = gpio_pin_set_dt(config->gpio, close_state);
    if (ret < 0) {
      LOG_ERR("Failed to close valve ID %d: %d", id, ret);

      digital_output_mutex_lock(valve);
      valve->status_mask |= (1U << id);
      digital_output_mutex_unlock(valve);

      return ret;
    }

    if (valve->callback != NULL) {
      timestamp = get_current_timestamp();
      valve->callback(config, false, timestamp, valve->status_mask);
    }

    LOG_INF("Valve ID %d closed", id);
  } else {
    digital_output_mutex_unlock(valve);
    LOG_DBG("Valve ID %d already closed", id);
  }

  return 0;
}

bool digital_output_get_status(digital_output_t *valve, int id) {
  int ret;
  int pin_value;
  bool is_open = false;
  const digital_output_config_list_t *config;

  if (valve == NULL) {
    return false;
  }

  config = find_digital_output_config(valve, id);
  if (config == NULL) {
    LOG_ERR("Valve ID %d not found in configuration", id);
    return false;
  }

  pin_value = gpio_pin_get_dt(config->gpio);
  if (pin_value < 0) {
    LOG_ERR("Failed to read valve ID %d status: %d", id, pin_value);
    return false;
  }

  if (config->active_high) {
    is_open = (pin_value == 1);
  } else {
    is_open = (pin_value == 0);
  }

  ret = digital_output_mutex_lock(valve);
  if (ret < 0) {
    return is_open;
  }

  if (is_open) {
    valve->status_mask |= (1U << id);
  } else {
    valve->status_mask &= ~(1U << id);
  }

  digital_output_mutex_unlock(valve);

  return is_open;
}

int digital_output_get_status_all(digital_output_t *valve, uint32_t *mask) {
  int ret;

  if (valve == NULL || mask == NULL) {
    return -EINVAL;
  }

  ret = digital_output_mutex_lock(valve);
  if (ret < 0) {
    return ret;
  }

  *mask = valve->status_mask;
  digital_output_mutex_unlock(valve);

  return 0;
}

int digital_output_force_set(digital_output_t *valve, uint32_t status_mask) {
  int id;
  int ret = -EINVAL;
  bool is_open;
  bool should_be_open;
  uint32_t current_mask;

  if (valve == NULL) {
    return ret;
  }

  ret = digital_output_get_status_all(valve, &current_mask);
  if (ret < 0) {
    return ret;
  }

  for (int i = 0; i < valve->config_count; i++) {
    id = valve->config_list[i].id;
    should_be_open = (status_mask & (1U << id)) ? true : false;
    is_open = (current_mask & (1U << id)) ? true : false;

    if (should_be_open && !is_open) {
      ret = digital_output_open(valve, id);
      if (ret < 0) {
        LOG_ERR("Failed to open valve ID %d during force set: %d", id, ret);
      }
    } else if (!should_be_open && is_open) {
      ret = digital_output_close(valve, id);
      if (ret < 0) {
        LOG_ERR("Failed to close valve ID %d during force set: %d", id, ret);
      }
    }
  }

  return 0;
}

int digital_output_show_list(digital_output_t *valve) {
  const digital_output_config_list_t *config;

  if (valve == NULL || valve->config_list == NULL) {
    return -EINVAL;
  }

  printk("\nValve list:\n");
  printk("----------------\n");

  for (int i = 0; i < valve->config_count; i++) {
    config = &valve->config_list[i];

    printk("ID: %-3d | Logic: %-9s | Description: %s\n", config->id,
           config->active_high ? "Active High" : "Active Low",
           config->description ? config->description : "No description");
  }

  printk("----------------\n");

  return 0;
}

int digital_output_show_active(digital_output_t *valve) {
  int ret;
  bool is_active;
  bool has_active;
  uint32_t status_mask;
  const digital_output_config_list_t *config;

  if (valve == NULL || valve->config_list == NULL) {
    return -EINVAL;
  }

  ret = digital_output_get_status_all(valve, &status_mask);
  if (ret < 0) {
    return ret;
  }

  printk("\nActive valves:\n");
  printk("----------------\n");

  has_active = false;

  for (int i = 0; i < valve->config_count; i++) {
    config = &valve->config_list[i];
    is_active = (status_mask & (1U << config->id)) ? true : false;

    if (is_active) {
      has_active = true;
      printk("ID: %-3d | Description: %s\n", config->id,
             config->description ? config->description : "No description");
    }
  }

  if (!has_active) {
    printk("No active valves.\n");
  }

  printk("----------------\n");

  return 0;
}