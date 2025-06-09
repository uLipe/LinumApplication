/**
 * @file digital_output.h
 */

#ifndef DIG_OUTPUT_H_
#define DIG_OUTPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOUT_MAX_COUNT 32

typedef struct {
  int id;                          /* Valve identifier */
  const struct gpio_dt_spec *gpio; /* GPIO specification */
  bool active_high;                /* true if valve activates on high signal, false for low */
  const char *description;         /* Optional description */
} digital_output_config_list_t;

typedef void (*digital_output_callback_t)(
    const digital_output_config_list_t *config, bool state, uint32_t timestamp,
    uint32_t all_states_mask);

typedef struct {
  uint32_t status_mask;               /* Mask with current valve states */
  const digital_output_config_list_t
      *config_list;                   /* List of valve configurations */
  uint16_t config_count;              /* Number of items in config list */
  struct k_mutex mutex;               /* Mutex for controlling access */
  digital_output_callback_t callback; /* Optional callback function */
} digital_output_t;

int digital_output_init(digital_output_t *valve,
                        const digital_output_config_list_t *config_list,
                        uint16_t config_count,
                        digital_output_callback_t callback);

int digital_output_open(digital_output_t *valve, int id);
int digital_output_close(digital_output_t *valve, int id);
bool digital_output_get_status(digital_output_t *valve, int id);
int digital_output_get_status_all(digital_output_t *valve, uint32_t *mask);
int digital_output_force_set(digital_output_t *valve, uint32_t status_mask);
int digital_output_show_list(digital_output_t *valve);
int digital_output_show_active(digital_output_t *valve);

#ifdef __cplusplus
}
#endif

#endif /* DIG_OUTPUT_H_ */