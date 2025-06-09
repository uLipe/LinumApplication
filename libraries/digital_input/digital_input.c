/**
 * @file digital_input.c
 */

#include "digital_input.h"
#include <string.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(bsp_dinput);

static int digital_input_mutex_lock(digital_input_t *input);
static int digital_input_mutex_unlock(digital_input_t *input);
static uint32_t get_current_timestamp(void);
static const digital_input_config_t *find_digital_input_config(
    digital_input_t *input, int id);
static void hysteresis_timer_callback(struct k_timer *timer);
static int digital_input_event_add(digital_input_t *input, int id, bool state, uint32_t timestamp);
static void input_change_work_handler(struct k_work *work);

static int digital_input_mutex_lock(digital_input_t *input) {
  return k_mutex_lock(&input->mutex, K_FOREVER);
}

static int digital_input_mutex_unlock(digital_input_t *input) {
  return k_mutex_unlock(&input->mutex);
}

static uint32_t get_current_timestamp(void) { 
  return k_uptime_get_32(); 
}

static const digital_input_config_t *find_digital_input_config(
    digital_input_t *input, int id) {
  if (input == NULL || input->config_list == NULL) {
    return NULL;
  }

  for (int i = 0; i < input->config_count; i++) {
    if (input->config_list[i].id == id) {
      return &input->config_list[i];
    }
  }

  return NULL;
}

// Função para adicionar evento à fila de mensagens
static int digital_input_event_add(digital_input_t *input, int id, bool state, uint32_t timestamp) {
  digital_input_event_t event;
  int ret;
  const digital_input_config_t *config;

  if (!input) {
    return -EINVAL;
  }

  config = find_digital_input_config(input, id);
  if (!config) {
    LOG_ERR("Input ID %d not found in configuration", id);
    return -EINVAL;
  }

  // Preencher a estrutura do evento
  event.input_id = id;
  event.state = state;
  event.timestamp = timestamp;
  event.status_mask = input->status_mask;
  event.config = config;

  // Enfileirar o evento (não bloquear se a fila estiver cheia)
  ret = k_msgq_put(&input->event_queue, &event, K_NO_WAIT);
  if (ret == 0) {
    // Se o evento foi enfileirado com sucesso, submeter o work para processamento
    k_work_submit(&input->input_change_work);
  } else {
    LOG_WRN("Failed to enqueue input event for ID %d, queue full or error: %d", id, ret);
  }

  return ret;
}

// Handler do work de eventos
static void input_change_work_handler(struct k_work *work) {
  digital_input_t *input = CONTAINER_OF(work, digital_input_t, input_change_work);
  digital_input_event_t event;
  
  // Processar todos os eventos da fila
  while (k_msgq_get(&input->event_queue, &event, K_NO_WAIT) == 0) {
    if (input->event_callback) {
      input->event_callback(&event);
    }
  }
}

static void hysteresis_timer_callback(struct k_timer *timer) {
  digital_input_timer_data_t *data = (digital_input_timer_data_t *)timer->user_data;
  digital_input_t *input = (digital_input_t *)data->input;
  int id = data->id;
  bool state = data->state;
  uint32_t timestamp = get_current_timestamp();
  uint32_t old_mask, new_mask;

  if (input == NULL) {
    return;
  }

  digital_input_mutex_lock(input);

  old_mask = input->status_mask;
  if (state) {
    input->status_mask |= (1U << id);
  } else {
    input->status_mask &= ~(1U << id);
  }
  new_mask = input->status_mask;

  if ((input->status_reported_mask & (1U << id)) != (new_mask & (1U << id))) {
    input->status_reported_mask =
        (input->status_reported_mask & ~(1U << id)) | (new_mask & (1U << id));

    digital_input_mutex_unlock(input);

    // Adicionar o evento à fila para processamento assíncrono
    if (input->event_callback) {
      digital_input_event_add(input, id, state, timestamp);
    }
  } else {
    digital_input_mutex_unlock(input);
  }
}

int digital_input_init(digital_input_t *input,
                      const digital_input_config_t *config_list,
                      uint16_t config_count,
                      digital_input_event_cb_t callback,
                      uint16_t queue_size) {
  int ret;

  if (input == NULL || config_list == NULL || config_count == 0 ||
      config_count > DIGITAL_INPUT_MAX_COUNT) {
    return -EINVAL;
  }
  
  // Validação do tamanho da fila
  if (queue_size == 0) {
    queue_size = 1;  // Tamanho mínimo
  } else if (queue_size > DIGITAL_INPUT_EVENT_QUEUE_SIZE) {
    queue_size = DIGITAL_INPUT_EVENT_QUEUE_SIZE;  // Limitar ao máximo definido
  }

  memset(input, 0, sizeof(digital_input_t));
  k_mutex_init(&input->mutex);
  
  // Inicializar a fila de mensagens para eventos
  k_msgq_init(&input->event_queue, 
              input->event_msgq_buffer,
              sizeof(digital_input_event_t), 
              queue_size);
  
  // Inicializar o work para processamento de eventos
  k_work_init(&input->input_change_work, input_change_work_handler);

  input->config_list = config_list;
  input->config_count = config_count;
  input->event_callback = callback;
  input->status_mask = 0;

  for (int i = 0; i < config_count; i++) {
    const struct gpio_dt_spec *gpio = config_list[i].gpio;

    if (!gpio_is_ready_dt(gpio)) {
      LOG_ERR("GPIO not ready for input ID %d", config_list[i].id);
      return -ENODEV;
    }

    ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
    if (ret < 0) {
      LOG_ERR("Failed to configure GPIO for input ID %d: %d", config_list[i].id,
              ret);
      return ret;
    }

    input->hysteresis[i].hysteresis_ms = 0;
    input->hysteresis[i].pending.id = config_list[i].id;
    input->hysteresis[i].pending.input = input;
    k_timer_init(&input->hysteresis[i].timer, hysteresis_timer_callback, NULL);
    k_timer_user_data_set(&input->hysteresis[i].timer,
                          &input->hysteresis[i].pending);

    int pin_value = gpio_pin_get_dt(gpio);
    if (pin_value >= 0) {
      bool active = (pin_value != 0) != config_list[i].active_high;
      if (active) {
        input->status_mask |= (1U << config_list[i].id);
      }
    }
  }

  input->status_reported_mask = input->status_mask;

  LOG_INF("Digital input system initialized with %d inputs", config_count);
  return 0;
}

// Inicialização com tamanho padrão
int digital_input_init_default(digital_input_t *input, const digital_input_config_t *config_list,
                             uint16_t config_count, digital_input_event_cb_t callback) {
  return digital_input_init(input, config_list, config_count, callback, DIGITAL_INPUT_EVENT_QUEUE_SIZE);
}

// Limpar a fila de eventos
int digital_input_flush_event_queue(digital_input_t *input) {
  if (input == NULL) {
    return -EINVAL;
  }
  
  k_msgq_purge(&input->event_queue);
  
  return 0;
}

// Liberação de recursos
int digital_input_deinit(digital_input_t *input) {
  if (input == NULL) {
    return -EINVAL;
  }
  
  // Parar todos os timers ativos
  for (int i = 0; i < input->config_count; i++) {
    k_timer_stop(&input->hysteresis[i].timer);
  }
  
  // Limpar a fila de eventos
  k_msgq_purge(&input->event_queue);
  
  return 0;
}

int digital_input_set_hysteresis(digital_input_t *input, int id,
                                uint32_t hysteresis_ms) {
  int ret;
  const digital_input_config_t *config;

  if (input == NULL) {
    return -EINVAL;
  }

  config = find_digital_input_config(input, id);
  if (config == NULL) {
    LOG_ERR("Input ID %d not found in configuration", id);
    return -EINVAL;
  }

  ret = digital_input_mutex_lock(input);
  if (ret < 0) {
    return ret;
  }

  for (int i = 0; i < input->config_count; i++) {
    if (input->config_list[i].id == id) {
      input->hysteresis[i].hysteresis_ms = hysteresis_ms;
      break;
    }
  }

  digital_input_mutex_unlock(input);

  LOG_INF("Hysteresis for input ID %d set to %u ms", id, hysteresis_ms);
  return 0;
}

int digital_input_get_hysteresis(digital_input_t *input, int id,
                                uint32_t *hysteresis_ms) {
  int ret;
  const digital_input_config_t *config;

  if (input == NULL || hysteresis_ms == NULL) {
    return -EINVAL;
  }

  config = find_digital_input_config(input, id);
  if (config == NULL) {
    LOG_ERR("Input ID %d not found in configuration", id);
    return -EINVAL;
  }

  ret = digital_input_mutex_lock(input);
  if (ret < 0) {
    return ret;
  }

  *hysteresis_ms = 0;
  for (int i = 0; i < input->config_count; i++) {
    if (input->config_list[i].id == id) {
      *hysteresis_ms = input->hysteresis[i].hysteresis_ms;
      break;
    }
  }

  digital_input_mutex_unlock(input);

  return 0;
}

int digital_input_get_status(digital_input_t *input, int id, uint32_t *status) {
  int ret;
  const digital_input_config_t *config;

  if (input == NULL || status == NULL) {
    return -EINVAL;
  }

  config = find_digital_input_config(input, id);
  if (config == NULL) {
    LOG_ERR("Input ID %d not found in configuration", id);
    return -EINVAL;
  }

  ret = digital_input_mutex_lock(input);
  if (ret < 0) {
    return ret;
  }

  *status = (input->status_mask & (1U << id)) ? 1 : 0;

  digital_input_mutex_unlock(input);

  return 0;
}

int digital_input_update_status(digital_input_t *input, int id,
                              uint32_t status) {
  int ret;
  uint32_t current_status;
  uint32_t timestamp;
  uint32_t new_mask;
  uint32_t hysteresis_ms = 0;
  const digital_input_config_t *config;

  if (input == NULL) {
    return -EINVAL;
  }

  config = find_digital_input_config(input, id);
  if (config == NULL) {
    LOG_ERR("Input ID %d not found in configuration", id);
    return -EINVAL;
  }

  ret = digital_input_get_status(input, id, &current_status);
  if (ret < 0) {
    return ret;
  }

  if (current_status == status) {
    k_timer_stop(&input->hysteresis[id].timer);
    return 0;
  }

  for (int i = 0; i < input->config_count; i++) {
    if (input->config_list[i].id == id) {
      hysteresis_ms = input->hysteresis[i].hysteresis_ms;
      k_timer_stop(&input->hysteresis[i].timer);

      if (hysteresis_ms > 0) {
        input->hysteresis[i].pending.state = status ? true : false;
        k_timer_start(&input->hysteresis[i].timer, K_MSEC(hysteresis_ms),
                      K_NO_WAIT);
        LOG_DBG("Started hysteresis timer for input ID %d, status %d, %u ms",
                id, status, hysteresis_ms);
        return 0;
      }

      break;
    }
  }

  ret = digital_input_mutex_lock(input);
  if (ret < 0) {
    return ret;
  }

  if (status) {
    input->status_mask |= (1U << id);
  } else {
    input->status_mask &= ~(1U << id);
  }

  new_mask = input->status_mask;

  if ((input->status_reported_mask & (1U << id)) != (new_mask & (1U << id))) {
    input->status_reported_mask = (input->status_reported_mask & ~(1U << id)) | (new_mask & (1U << id));
    digital_input_mutex_unlock(input);

    // Adicionar o evento à fila para processamento assíncrono
    if (input->event_callback) {
      timestamp = get_current_timestamp();
      digital_input_event_add(input, id, status ? true : false, timestamp);
    }
  } else {
    digital_input_mutex_unlock(input);
  }

  return 0;
}

int digital_input_set_all_state(digital_input_t *input, uint32_t mask) {
  int ret;

  if (input == NULL) {
    return -EINVAL;
  }

  ret = digital_input_mutex_lock(input);
  if (ret < 0) {
    return ret;
  }

  input->status_mask = mask;
  input->status_reported_mask = mask;

  digital_input_mutex_unlock(input);

  LOG_INF("All input states set to 0x%08X", mask);
  return 0;
}

int digital_input_get_status_all(digital_input_t *input, uint32_t *mask) {
  int ret;

  if (input == NULL || mask == NULL) {
    return -EINVAL;
  }

  ret = digital_input_mutex_lock(input);
  if (ret < 0) {
    return ret;
  }

  *mask = input->status_mask;

  digital_input_mutex_unlock(input);

  return 0;
}

int digital_input_show_list(digital_input_t *input) {
  uint32_t hysteresis;
  const digital_input_config_t *config;

  if (input == NULL || input->config_list == NULL) {
    return -EINVAL;
  }

  printk("\nInput list:\n");
  printk("----------------\n");

  for (int i = 0; i < input->config_count; i++) {
    config = &input->config_list[i];

    digital_input_get_hysteresis(input, config->id, &hysteresis);

    printk("ID: %-3d | Logic: %-9s | Hysteresis: %-5u ms | Description: %s\n",
           config->id, config->active_high ? "Active High" : "Active Low",
           hysteresis,
           config->description ? config->description : "No description");
  }

  printk("----------------\n");

  return 0;
}

int digital_input_show_active(digital_input_t *input) {
  int ret;
  bool is_active;
  bool has_active;
  uint32_t hysteresis;
  uint32_t status_mask;
  const digital_input_config_t *config;

  if (input == NULL || input->config_list == NULL) {
    return -EINVAL;
  }

  ret = digital_input_get_status_all(input, &status_mask);
  if (ret < 0) {
    return ret;
  }

  printk("\nActive inputs:\n");
  printk("----------------\n");

  has_active = false;

  for (int i = 0; i < input->config_count; i++) {
    config = &input->config_list[i];
    is_active = (status_mask & (1U << config->id)) ? true : false;

    if (is_active) {
      has_active = true;
      digital_input_get_hysteresis(input, config->id, &hysteresis);

      printk("ID: %-3d | Hysteresis: %-5u ms | Description: %s\n", config->id,
             hysteresis,
             config->description ? config->description : "No description");
    }
  }

  if (!has_active) {
    printk("No active inputs.\n");
  }

  printk("----------------\n");

  return 0;
}