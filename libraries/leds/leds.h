/**
 * @file leds.h
 */

 #ifndef LEDS_H_
 #define LEDS_H_
 
 #include <stdbool.h>
 #include <stdint.h>
 #include <zephyr/drivers/gpio.h>
 #include <zephyr/kernel.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define LED_MAX_COUNT 32
 
 typedef enum {
     LED_ACTION_OFF = 0,         /* Desliga o LED */
     LED_ACTION_ON,              /* Liga o LED */
     LED_ACTION_TOGGLE,          /* Inverte o estado do LED */
     LED_ACTION_BLINK_SLOW,      /* Pisca lentamente */
     LED_ACTION_BLINK_FAST,      /* Pisca rapidamente */
     LED_ACTION_HEARTBEAT        /* Pisca em padrão de batimento cardíaco */
 } led_action_t;
 
 typedef struct {
     int id;                          /* LED identifier */
     const struct gpio_dt_spec *gpio; /* GPIO specification */
     bool active_high;                /* true if LED activates on high signal, false for low */
     const char *description;         /* Optional description */
 } led_config_list_t;
 
 typedef void (*led_callback_t)(
     const led_config_list_t *config, bool state, uint32_t timestamp,
     uint32_t all_states_mask);
 
 typedef struct {
     uint32_t status_mask;           /* Mask with current LED states */
     uint32_t on_mask;               /* Mask with LEDs that are solidly on */
     uint32_t blink_slow_mask;       /* Mask with LEDs that are blinking slowly */
     uint32_t blink_fast_mask;       /* Mask with LEDs that are blinking quickly */
     uint32_t heartbeat_mask;        /* Mask with LEDs that are in heartbeat mode */
     const led_config_list_t *config_list; /* List of LED configurations */
     uint16_t config_count;          /* Number of items in config list */
     struct k_mutex mutex;           /* Mutex for controlling access */
     struct k_timer timer;           /* Timer for blinking patterns */
     led_callback_t callback;        /* Optional callback function */
 } led_control_t;
 
 int led_init(led_control_t *led_control, 
              const led_config_list_t *config_list,
              uint16_t config_count, 
              led_callback_t callback);
 
 int led_set(led_control_t *led_control, int id, led_action_t action);
 int led_set_mask(led_control_t *led_control, uint32_t mask, led_action_t action);
 bool led_get_status(led_control_t *led_control, int id);
 int led_get_status_all(led_control_t *led_control, uint32_t *mask);
 int led_show_list(led_control_t *led_control);
 int led_show_active(led_control_t *led_control);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* LEDS_H_ */