#include "app_ext_flash.h"
#include "app_icon.h"
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>

int ext_mem_test(void) {
  printk("Ext flash buffer 1 local: %p\n", nor_glyph_bitmap);
  printk("Ext flash buffer 2 local: %p\n", nor_hello_msg);
  // if (!strcmp(nor_hello_msg, "A caminhada é longa, mas o resultado faz cada
  // passo valer a pena!\0"))
  // {
  //     printk("Ext flash test with success\n");
  //     return 0;
  // }
  printk("Ext flash test fail\n");

  return -EIO;
}
