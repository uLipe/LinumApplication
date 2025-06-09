#include "app_ext_flash.h"
#include "app_sdram.h"
#include "app_version.h"
#include "buzzer_lib.h"
#include "database.h"
#include "eeprom_lib.h"
#include "eth_lib.h"
#include "lcd_lib.h"
#include "leds_lib.h"
#include "rtc_lib.h"
#include "setup_database.h"
#include "slave_modbus.h"
#include <zephyr/kernel.h>

#include "mask_format.h"
#include "sdcard_lib.h"
#include "string_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(void) {
  char *buf;
  enum leds_lib_channel channel = LED_BLUE;

  k_msleep(1000);

  printk(">> Project: ZPHR-STM32-0003 - v1.0.0<<\r\n");

  rtc_init();
  buzzer_init();
  lcd_init();
  eeprom_lib_init();
  leds_lib_init();
  lcd_bklight_set_percent(50);
  // sdcard_init();

  buf = rtc_format_datetime(NULL);
  printk("Time: %s\n", buf);

  // eeprom_test();
  // buzzer_test(2);
  // lcd_bklight_test(2);
  formatter_test();
  mask_formatter_test();
  sdcard_test();
  sdram_test();
  // ext_mem_test();
  lcd_lvgl_demo();
  eth_init();

  setup_database_init();
  slave_modbus_init();

  // buzzer_ringotne_test();

  uint8_t cnt = 0;
  uint8_t cnt2 = 0;
  char nome[40];
  char nome2[40];
  while (1) {
    cnt2 = cnt++;
    snprintf(nome, sizeof(nome), "valor: %d", cnt2);
    db_acc_set_u8(ACC_LEVEL_FACTORY, GROUP_PROC_VAR, PROC_VAR_MDB_IQC, cnt2);
    db_acc_set_str(ACC_LEVEL_FACTORY, GROUP_SYS_OTA_CONF, PARAM_CNFG_OTA_FILE,
                   nome, sizeof(nome));
    k_msleep(1000);

    cnt2 = 0;
    db_acc_get_u8(ACC_LEVEL_FACTORY, GROUP_PROC_VAR, PROC_VAR_MDB_IQC, &cnt2);
    db_acc_get_str(ACC_LEVEL_FACTORY, GROUP_SYS_OTA_CONF, PARAM_CNFG_OTA_FILE,
                   nome2, sizeof(nome2));
    printk("%s\n", nome2);

    channel = channel >= LED_MAX ? LED_BLUE : channel + 1;
    leds_lib_set(channel, true);

    leds_lib_clear();
  }

  return 0;
}
