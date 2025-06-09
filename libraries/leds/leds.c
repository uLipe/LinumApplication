/**
 * @file leds.c
 */

 #include "leds.h"
 #include <zephyr/logging/log.h>
 
 #define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
 LOG_MODULE_REGISTER(bsp_leds);
 
 /* Blinking intervals (in milliseconds) */
 #define LED_BLINK_SLOW_INTERVAL       700
 #define LED_BLINK_FAST_INTERVAL       200
 
 /* Heartbeat pattern intervals (in milliseconds) */
 #define LED_HEARTBEAT_PHASE_1         100
 #define LED_HEARTBEAT_PHASE_2         200
 #define LED_HEARTBEAT_PHASE_3         100
 #define LED_HEARTBEAT_PHASE_4        1200
 
 /* Timer interval in milliseconds */
 #define LED_TIMER_INTERVAL            10
 
 static void led_timer_handler(struct k_timer *timer);
 static int led_mutex_lock(led_control_t *led_control);
 static int led_mutex_unlock(led_control_t *led_control);
 static uint32_t get_current_timestamp(void);
 static const led_config_list_t *find_led_config(led_control_t *led_control, int id);
 static void update_led_outputs(led_control_t *led_control);
 
 static int led_mutex_lock(led_control_t *led_control) {
     return k_mutex_lock(&led_control->mutex, K_FOREVER);
 }
 
 static int led_mutex_unlock(led_control_t *led_control) {
     return k_mutex_unlock(&led_control->mutex);
 }
 
 static uint32_t get_current_timestamp(void) {
     return k_uptime_get_32();
 }
 
 static const led_config_list_t *find_led_config(led_control_t *led_control, int id) {
     if (led_control == NULL || led_control->config_list == NULL) {
         return NULL;
     }
 
     for (int i = 0; i < led_control->config_count; i++) {
         if (led_control->config_list[i].id == id) {
             return &led_control->config_list[i];
         }
     }
 
     return NULL;
 }
 
 static void update_led_physical_state(led_control_t *led_control, int id, bool state) {
     const led_config_list_t *config = find_led_config(led_control, id);
     
     if (config == NULL) {
         return;
     }
 
     int gpio_state = config->active_high ? (state ? 1 : 0) : (state ? 0 : 1);
     gpio_pin_set_dt(config->gpio, gpio_state);
 }
 
 static void update_led_outputs(led_control_t *led_control) {
    int id;
    bool is_on;
    uint32_t active_leds = led_control->status_mask;
     
     for (int i = 0; i < led_control->config_count; i++) {
         id = led_control->config_list[i].id;
         is_on = (active_leds & (1U << id)) ? true : false;
         update_led_physical_state(led_control, id, is_on);
     }
 }
 
 static void led_timer_handler(struct k_timer *timer) {
     led_control_t *led_control = CONTAINER_OF(timer, led_control_t, timer);
     static uint16_t slow_counter = 0;
     static uint16_t fast_counter = 0;
     static uint16_t heartbeat_counter = 0;
     static uint8_t heartbeat_phase = 0;
     bool toggle_slow = false;
     bool toggle_fast = false;
     bool toggle_heartbeat = false;
     
     /* Increment counters */
     slow_counter += LED_TIMER_INTERVAL;
     fast_counter += LED_TIMER_INTERVAL;
     heartbeat_counter += LED_TIMER_INTERVAL;
     
     /* Check for slow blink toggle */
     if (slow_counter >= LED_BLINK_SLOW_INTERVAL) {
         slow_counter = 0;
         toggle_slow = true;
     }
     
     /* Check for fast blink toggle */
     if (fast_counter >= LED_BLINK_FAST_INTERVAL) {
         fast_counter = 0;
         toggle_fast = true;
     }
     
     /* Check for heartbeat pattern transitions */
     if (heartbeat_phase == 0 && heartbeat_counter >= LED_HEARTBEAT_PHASE_1) {
         heartbeat_counter = 0;
         heartbeat_phase = 1;
         toggle_heartbeat = true;
     } else if (heartbeat_phase == 1 && heartbeat_counter >= LED_HEARTBEAT_PHASE_2) {
         heartbeat_counter = 0;
         heartbeat_phase = 2;
         toggle_heartbeat = true;
     } else if (heartbeat_phase == 2 && heartbeat_counter >= LED_HEARTBEAT_PHASE_3) {
         heartbeat_counter = 0;
         heartbeat_phase = 3;
         toggle_heartbeat = true;
     } else if (heartbeat_phase == 3 && heartbeat_counter >= LED_HEARTBEAT_PHASE_4) {
         heartbeat_counter = 0;
         heartbeat_phase = 0;
         toggle_heartbeat = true;
     }
     
     /* Acquire mutex for updating LED states */
     if (led_mutex_lock(led_control) < 0) {
         return;
     }
     
     /* Handle slow blinking LEDs */
     if (toggle_slow && led_control->blink_slow_mask) {
         led_control->status_mask ^= led_control->blink_slow_mask;
     }
     
     /* Handle fast blinking LEDs */
     if (toggle_fast && led_control->blink_fast_mask) {
         led_control->status_mask ^= led_control->blink_fast_mask;
     }
     
     /* Handle heartbeat pattern LEDs */
     if (toggle_heartbeat && led_control->heartbeat_mask) {
         /* In phases 0 and 2, LED is off; in phases 1 and 3, LED is on */
         if (heartbeat_phase == 0 || heartbeat_phase == 2) {
             led_control->status_mask &= ~led_control->heartbeat_mask;
         } else {
             led_control->status_mask |= led_control->heartbeat_mask;
         }
     }
     
     /* Update physical LED states */
     update_led_outputs(led_control);
     
     /* Release mutex */
     led_mutex_unlock(led_control);
 }
 
 int led_init(led_control_t *led_control, 
              const led_config_list_t *config_list,
              uint16_t config_count, 
              led_callback_t callback) {
     int ret;
 
     if (led_control == NULL || config_list == NULL || config_count == 0 ||
         config_count > LED_MAX_COUNT) {
         return -EINVAL;
     }
 
     memset(led_control, 0, sizeof(led_control_t));
     k_mutex_init(&led_control->mutex);
 
     led_control->config_list = config_list;
     led_control->config_count = config_count;
     led_control->callback = callback;
 
     for (int i = 0; i < config_count; i++) {
         const struct gpio_dt_spec *gpio = config_list[i].gpio;
 
         if (!gpio_is_ready_dt(gpio)) {
             LOG_ERR("GPIO not ready for LED ID %d", config_list[i].id);
             return -ENODEV;
         }
 
         ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
         if (ret < 0) {
             LOG_ERR("Failed to configure GPIO for LED ID %d: %d", config_list[i].id,
                   ret);
             return ret;
         }
 
         int initial_state = config_list[i].active_high ? 0 : 1;
         ret = gpio_pin_set_dt(gpio, initial_state);
         if (ret < 0) {
             LOG_ERR("Failed to set initial state for LED ID %d: %d",
                   config_list[i].id, ret);
             return ret;
         }
     }
 
     /* Initialize and start the timer for blinking patterns */
     k_timer_init(&led_control->timer, led_timer_handler, NULL);
     k_timer_start(&led_control->timer, K_MSEC(LED_TIMER_INTERVAL), K_MSEC(LED_TIMER_INTERVAL));
 
     LOG_INF("LED system initialized with %d LEDs", config_count);
     return 0;
 }
 
 int led_set(led_control_t *led_control, int id, led_action_t action) {
     uint32_t mask = (1U << id);
     return led_set_mask(led_control, mask, action);
 }
 
 int led_set_mask(led_control_t *led_control, uint32_t mask, led_action_t action) {
     int ret;
     uint32_t timestamp;
 
     if (led_control == NULL) {
         return -EINVAL;
     }
 
     ret = led_mutex_lock(led_control);
     if (ret < 0) {
         return ret;
     }
 
     /* Update LED mode masks based on the requested action */
     switch (action) {
         case LED_ACTION_OFF:
             led_control->on_mask &= ~mask;
             led_control->blink_slow_mask &= ~mask;
             led_control->blink_fast_mask &= ~mask;
             led_control->heartbeat_mask &= ~mask;
             led_control->status_mask &= ~mask;
             break;
 
         case LED_ACTION_ON:
             led_control->on_mask |= mask;
             led_control->blink_slow_mask &= ~mask;
             led_control->blink_fast_mask &= ~mask;
             led_control->heartbeat_mask &= ~mask;
             led_control->status_mask |= mask;
             break;
 
         case LED_ACTION_TOGGLE:
             led_control->on_mask ^= mask;
             led_control->blink_slow_mask &= ~mask;
             led_control->blink_fast_mask &= ~mask;
             led_control->heartbeat_mask &= ~mask;
             led_control->status_mask ^= mask;
             break;
 
         case LED_ACTION_BLINK_SLOW:
             led_control->on_mask &= ~mask;
             led_control->blink_slow_mask |= mask;
             led_control->blink_fast_mask &= ~mask;
             led_control->heartbeat_mask &= ~mask;
             break;
 
         case LED_ACTION_BLINK_FAST:
             led_control->on_mask &= ~mask;
             led_control->blink_slow_mask &= ~mask;
             led_control->blink_fast_mask |= mask;
             led_control->heartbeat_mask &= ~mask;
             break;
 
         case LED_ACTION_HEARTBEAT:
             led_control->on_mask &= ~mask;
             led_control->blink_slow_mask &= ~mask;
             led_control->blink_fast_mask &= ~mask;
             led_control->heartbeat_mask |= mask;
             break;
 
         default:
             led_mutex_unlock(led_control);
             return -EINVAL;
     }
 
     /* Update physical LED states */
     update_led_outputs(led_control);
 
     /* Call the callback if provided */
     if (led_control->callback != NULL) {
         for (int i = 0; i < led_control->config_count; i++) {
             int id = led_control->config_list[i].id;
             if (mask & (1U << id)) {
                 bool state = (led_control->status_mask & (1U << id)) ? true : false;
                 timestamp = get_current_timestamp();
                 led_control->callback(&led_control->config_list[i], state, 
                                      timestamp, led_control->status_mask);
             }
         }
     }
 
     led_mutex_unlock(led_control);
     return 0;
 }
 
 bool led_get_status(led_control_t *led_control, int id) {
     int ret;
     int pin_value;
     bool is_on = false;
     const led_config_list_t *config;
 
     if (led_control == NULL) {
         return false;
     }
 
     config = find_led_config(led_control, id);
     if (config == NULL) {
         LOG_ERR("LED ID %d not found in configuration", id);
         return false;
     }
 
     pin_value = gpio_pin_get_dt(config->gpio);
     if (pin_value < 0) {
         LOG_ERR("Failed to read LED ID %d status: %d", id, pin_value);
         return false;
     }
 
     if (config->active_high) {
         is_on = (pin_value == 1);
     } else {
         is_on = (pin_value == 0);
     }
 
     ret = led_mutex_lock(led_control);
     if (ret < 0) {
         return is_on;
     }
 
     /* Update status mask to match actual hardware state */
     if (is_on) {
         led_control->status_mask |= (1U << id);
     } else {
         led_control->status_mask &= ~(1U << id);
     }
 
     led_mutex_unlock(led_control);
     return is_on;
 }
 
 int led_get_status_all(led_control_t *led_control, uint32_t *mask) {
     int ret;
 
     if (led_control == NULL || mask == NULL) {
         return -EINVAL;
     }
 
     ret = led_mutex_lock(led_control);
     if (ret < 0) {
         return ret;
     }
 
     *mask = led_control->status_mask;
     led_mutex_unlock(led_control);
 
     return 0;
 }
 
 int led_show_list(led_control_t *led_control) {
     const led_config_list_t *config;
 
     if (led_control == NULL || led_control->config_list == NULL) {
         return -EINVAL;
     }
 
     printk("\nLED list:\n");
     printk("----------------\n");
 
     for (int i = 0; i < led_control->config_count; i++) {
         config = &led_control->config_list[i];
 
         printk("ID: %-3d | Logic: %-9s | Description: %s\n", config->id,
              config->active_high ? "Active High" : "Active Low",
              config->description ? config->description : "No description");
     }
 
     printk("----------------\n");
     return 0;
 }
 
 int led_show_active(led_control_t *led_control) {
     int ret;
     bool is_active;
     bool has_active;
     uint32_t status_mask;
     const led_config_list_t *config;
 
     if (led_control == NULL || led_control->config_list == NULL) {
         return -EINVAL;
     }
 
     ret = led_get_status_all(led_control, &status_mask);
     if (ret < 0) {
         return ret;
     }
 
     printk("\nActive LEDs:\n");
     printk("----------------\n");
 
     has_active = false;
 
     for (int i = 0; i < led_control->config_count; i++) {
         config = &led_control->config_list[i];
         is_active = (status_mask & (1U << config->id)) ? true : false;
 
         if (is_active) {
             has_active = true;
             printk("ID: %-3d | Description: %s\n", config->id,
                  config->description ? config->description : "No description");
         }
     }
 
     if (!has_active) {
         printk("No active LEDs.\n");
     }
 
     printk("----------------\n");
     return 0;
 }