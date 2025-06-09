#include "leds_lib.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

static int leds_gpio_configure(const char *name, struct gpio_dt_spec *gpio,
                               gpio_flags_t flags);

static struct gpio_dt_spec led_r = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led_g = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static struct gpio_dt_spec led_b = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static int leds_gpio_configure(const char *name, struct gpio_dt_spec *gpio,
                               gpio_flags_t flags) {
  int ret;

  ret = device_is_ready(gpio->port);
  if (ret < 0) {
    printk("%s not ready!\r\n", name);
    return ret;
  }

  ret = gpio_pin_configure_dt(gpio, flags);
  if (ret < 0) {
    printk("%s config failure, 0x%X\r\n", name, ret);
  }

  return ret;
}

int leds_lib_init(void) {
  int result;

  result = leds_gpio_configure("led blue", &led_b, GPIO_OUTPUT_ACTIVE);
  if (result < 0) {
    return result;
  }

  result = leds_gpio_configure("led green", &led_g, GPIO_OUTPUT_ACTIVE);
  if (result < 0) {
    return result;
  }

  result = leds_gpio_configure("led red", &led_r, GPIO_OUTPUT_ACTIVE);
  if (result < 0) {
    return result;
  }

  leds_lib_set(LED_BLUE, false);
  leds_lib_set(LED_GREEN, false);
  leds_lib_set(LED_RED, false);

  return result;
}

int leds_lib_set(enum leds_lib_channel led, bool enable) {
  int result;

  switch (led) {
  case LED_BLUE:
    result = gpio_pin_set_dt(&led_b, enable);
    break;

  case LED_GREEN:
    result = gpio_pin_set_dt(&led_g, enable);
    break;

  case LED_RED:
    result = gpio_pin_set_dt(&led_r, enable);
    break;

  case LED_WHITE:
    result = gpio_pin_set_dt(&led_b, enable);
    result = gpio_pin_set_dt(&led_g, enable);
    result = gpio_pin_set_dt(&led_r, enable);
    break;

  case LED_YELLOW:
    result = gpio_pin_set_dt(&led_g, enable);
    result = gpio_pin_set_dt(&led_r, enable);

  default:
    result = -EIO;
    break;
  }

  return result;
}

void leds_lib_clear(void) {
  leds_lib_set(LED_BLUE, 0);
  leds_lib_set(LED_GREEN, 0);
  leds_lib_set(LED_RED, 0);
}
