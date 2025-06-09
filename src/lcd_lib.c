

#include "lcd_lib.h"
#include <lv_demos.h>
#include <lvgl.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

#define NUM_STEPS 50U
#define SLEEP_MSEC 100U

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct pwm_dt_spec g_backlight =
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_backlight0));

int lcd_init(void) {
  int err;

  err = device_is_ready(display_dev);
  if (!err) {
    printk("Device not ready, aborting test");
    return err;
  }

  err = pwm_is_ready_dt(&g_backlight);
  if (!err) {
    printk("Error: PWM device %s is not ready\n", g_backlight.dev->name);
    return -EIO;
  }

  return 0;
}

int lcd_bklight_set_percent(int percent) {
  uint32_t pulse_width;
  int ret;

  // Verifica se a porcentagem está dentro dos limites válidos
  if (percent < 0 || percent > 100) {
    return -EINVAL; // Retorna erro de valor inválido
  }

  // Calcula a largura do pulso baseado na porcentagem
  pulse_width = (g_backlight.period * percent) / 100;

  // Define o pulso PWM
  ret = pwm_set_pulse_dt(&g_backlight, pulse_width);
  if (ret) {
    printk("Error %d: failed to set pulse width\n", ret);
    return ret;
  }

  printk("PWM set to %d%%\n", percent);
  return 0;
}

int lcd_bklight_test(int test_cycles) {
  int count = 0;
  int percent = 0U;
  int ret;

  do {
    count++;
    percent = 0;

    while (percent < 100) {
      percent++;

      ret = lcd_bklight_set_percent(percent);
      if (ret) {
        return ret;
      }

      k_sleep(K_MSEC(SLEEP_MSEC));
    }
  } while (count < test_cycles);

  return 0;
}

int lcd_lvgl_demo(void) {
#if defined(CONFIG_LV_USE_DEMO_MUSIC)
  lv_demo_music();
#elif defined(CONFIG_LV_USE_DEMO_BENCHMARK)
  lv_demo_benchmark();
#elif defined(CONFIG_LV_USE_DEMO_STRESS)
  lv_demo_stress();
#elif defined(CONFIG_LV_USE_DEMO_WIDGETS)
  lv_demo_widgets();
#else
#error Enable one of the demos CONFIG_LV_USE_DEMO_MUSIC, CONFIG_LV_USE_DEMO_BENCHMARK ,\
	CONFIG_LV_USE_DEMO_STRESS, or CONFIG_LV_USE_DEMO_WIDGETS
#endif

  lv_timer_handler();
  display_blanking_off(display_dev);
  
  while (1) {
    uint32_t sleep_ms = lv_timer_handler();
    k_msleep(MIN(sleep_ms, INT32_MAX));
  }

  return 0;
}
