
#include "buzzer_lib.h"
#include "ringtone.h"
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

#define NUM_STEPS 50U
#define SLEEP_MSEC 25U

// Tipo para notificacoes de buzzer
typedef enum {
  RINGTONE_TOUCH_PRESSED,
  RINGTONE_ALARM1,
  RINGTONE_ALARM2,
} ringtone_notification_type_t;

// Exemplo de ringtone personalizado
static const char *MARIO_RTTTL =
    "Super Mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,"
    "8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,"
    "8a6,16f6,8g6,8e6,16c6,16d6,8b";
static const char *TOUCHSCREEN_PRESSED_RTTTL = "Touch:d=16,o=6,b=180:c,p";
static const char *TOUCHSCREEN_LONG_PRESSED_RTTTL = "beep:d=4,o=5,b=100:c6";
static const char *ALARM1_RTTTL = "Alarm1:d=8,o=5,b=160:c6,p,c6,p,c6,p,c6";
static const char *ALARM2_RTTTL = "Alarm2:d=4,o=5,b=200:c6,g5,c6,g5,c6,g5,c6";

static ringstones_t g_ringstones;
static const struct pwm_dt_spec g_buzzer =
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_buzzer0));

int buzzer_init(void) {
  int err;

  err = pwm_is_ready_dt(&g_buzzer);
  if (!err) {
    printk("Error: PWM device %s is not ready\n", g_buzzer.dev->name);
    return -EIO;
  }

  pwm_set_pulse_dt(&g_buzzer, 0);

  return 0;
}

int buzzer_set_percent(int percent) {
  uint32_t pulse_width;
  int ret;

  // Verifica se a porcentagem está dentro dos limites válidos
  if (percent < 0 || percent > 100) {
    return -EINVAL; // Retorna erro de valor inválido
  }

  // Calcula a largura do pulso baseado na porcentagem
  pulse_width = (g_buzzer.period * percent) / 100;

  // Define o pulso PWM
  ret = pwm_set_pulse_dt(&g_buzzer, pulse_width);
  if (ret) {
    printk("Error %d: failed to set pulse width\n", ret);
    return ret;
  }

  printk("PWM set to %d%%\n", percent);
  return 0;
}

// int buzzer_test(void)
// {
//   uint32_t pulse_width = 0U;
//   uint32_t step = g_buzzer.period / NUM_STEPS;
//   uint8_t dir = 1U;
//   int ret;

//   while (1)
//   {
//     ret = pwm_set_pulse_dt(&g_buzzer, pulse_width);
//     if (ret)
//     {
//       printk("Error %d: failed to set pulse width\n", ret);
//       return 0;
//     }
//     printk("Using pulse width %d%%\n", 100 * pulse_width / g_buzzer.period);

//     if (dir)
//     {
//       pulse_width += step;
//       if (pulse_width >= g_buzzer.period)
//       {
//         pulse_width = g_buzzer.period - step;
//         dir = 0U;
//       }
//     }
//     else
//     {
//       if (pulse_width >= step)
//       {
//         pulse_width -= step;
//       }
//       else
//       {
//         pulse_width = step;
//         dir = 1U;
//       }
//     }

//     k_sleep(K_MSEC(SLEEP_MSEC));
//   }
//   return 0;
// }

int buzzer_test(int test_cycles) {
  int count = 0;
  int percent = 0U;
  int ret;

  do {
    count++;
    percent = 0;

    while (percent < 100) {
      percent++;

      ret = buzzer_set_percent(percent);
      if (ret) {
        return ret;
      }

      k_sleep(K_MSEC(SLEEP_MSEC));
    }
  } while (count < test_cycles);

  buzzer_set_percent(0);

  return 0;
}

int buzzer_play_notification(ringtone_notification_type_t type) {
  if (ringtones_is_playing(&g_ringstones)) {
    ringtones_stop(&g_ringstones);
  }

  switch (type) {
  case RINGTONE_TOUCH_PRESSED:
    g_ringstones.config.rtttl_string = MARIO_RTTTL;
    break;
  case RINGTONE_ALARM1:
    g_ringstones.config.rtttl_string = ALARM1_RTTTL;
    break;
  case RINGTONE_ALARM2:
    g_ringstones.config.rtttl_string = ALARM2_RTTTL;
    break;
  default:
    return -EINVAL;
  }

  return ringtones_play(&g_ringstones);
}

int Buzzer_PlayCustom(const char *rtttl_string) {
  if (rtttl_string == NULL) {
    return -EINVAL;
  }

  if (ringtones_is_playing(&g_ringstones)) {
    ringtones_stop(&g_ringstones);
  }

  g_ringstones.config.rtttl_string = rtttl_string;

  return ringtones_play(&g_ringstones);
}

int buzzer_ringotne_test(void) {
  int ret;

  printk("Ringtone example started\n");

  ret = buzzer_init();
  if (ret) {
    return ret;
  }

  ret = ringtones_init(&g_ringstones, &g_buzzer, TOUCHSCREEN_PRESSED_RTTTL, 4,
                       5, 100);
  if (ret) {
    printk("Failed to initialize ringtones: %d", ret);
    return ret;
  }

  // Toca um toque na tela
  printk("Playing touch sound\n");

  // Toca um ringtone personalizado
  printk("Playing custom ringtone\n");
  Buzzer_PlayCustom(MARIO_RTTTL);
  ringtones_wait_done(&g_ringstones, K_FOREVER);

  k_sleep(K_SECONDS(10));
  buzzer_play_notification(RINGTONE_TOUCH_PRESSED);

  printk("Ringtone example completed\n");
  return ret;
}
