#include "eeprom_lib.h"
#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/kernel.h>

static int mutex_lock(void);
static void mutex_unlock(void);

static bool eeprom_init = false;
static struct k_mutex eeprom_mutex;
const struct device *const dev_eepromm = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

static int mutex_lock(void) {
  return k_mutex_lock(&eeprom_mutex, K_MSEC(10000));
}

static void mutex_unlock(void) { k_mutex_unlock(&eeprom_mutex); }

int eeprom_lib_init(void) {
  if (!device_is_ready(dev_eepromm)) {
    printk("Device \"%s\" is not ready\n", dev_eepromm->name);
    mutex_unlock();
    return -EIO;
  }

  eeprom_init = true;
  printk("Found fram device \"%s\"\n", dev_eepromm->name);

  mutex_unlock();
  return 0;
}

int eeprom_lib_write(size_t offset, const void *buf, size_t buflen) {
  int rc;

  if (!eeprom_init) {
    return -EIO;
  }

  rc = mutex_lock();
  if (rc) {
    return rc;
  }

  if (dev_eepromm == NULL) {
    printk("fram device is not initialized.\n");
    mutex_unlock();
    return -EINVAL;
  }

  rc = eeprom_write(dev_eepromm, offset, buf, buflen);
  if (rc < 0) {
    printk("Error: Couldn't write fram: err:%d.\n", rc);
  }

  mutex_unlock();
  return rc;
}

int eeprom_lib_read(size_t offset, void *buf, size_t buflen) {
  int rc = 0;

  if (!eeprom_init) {
    return -EIO;
  }

  rc = mutex_lock();
  if (rc) {
    return rc;
  }

  rc = eeprom_read(dev_eepromm, offset, buf, buflen);
  if (rc < 0) {
    printk("Error: Couldn't write fram: err:%d.\n", rc);
  } else {
    printk("Data read from fram.\n");
  }

  mutex_unlock();
  return rc;
}

int eeprom_test(void) {
  int ret;
  char msg[] =
      "A caminhada é longa, mas o resultado faz cada passo valer a pena!\0";
  char read_msg[128];

  memset(read_msg, 0, sizeof(read_msg));

  ret = eeprom_lib_write(0, msg, sizeof(msg));
  if (!ret) {
    ret = eeprom_lib_read(0, read_msg, sizeof(msg));
    if (!ret) {
      printk("eeprom read message: %s\n", read_msg);
    }
  }

  return ret;
}
