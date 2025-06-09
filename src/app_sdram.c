/*
 * Copyright (c) 2022 Circuit Dojo LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_sdram.h"
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>

#define USER_HEAP_SIZE (1024 * 4)
#define USER_HEAP_MEM_ATTRIBUTES Z_GENERIC_SECTION(.jorge_heap)

static uint8_t user_memory[USER_HEAP_SIZE] USER_HEAP_MEM_ATTRIBUTES;
static struct k_heap user_heap;

static void *os_mem_alloc(int size);
static void os_mem_free(void *ptr);
static int os_mem_heap_init(void);

static void *os_mem_alloc(int size) {
  return (k_heap_alloc(&user_heap, size, K_NO_WAIT));
}

static void os_mem_free(void *ptr) { k_heap_free(&user_heap, ptr); }

static int os_mem_heap_init(void) {
  k_heap_init(&user_heap, &user_memory[0], USER_HEAP_SIZE);
  return 0;
}

SYS_INIT(os_mem_heap_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

int sdram_test(void) {
  char *sdram_buffer;
  const char *test_patterns[] = {
      "DeadBeefCafeBabe", // Padrão hexadecimal comum
      "0xFF00xFF00xFF00", // Alternando bytes
      "0000000000000000", // Todos zeros
      "1111111111111111", // Todos uns
      "0101010101010101", // Alternando 0 e 1
      "1010101010101010", // Alternando 1 e 0
  };
  for (int i = 0; i < sizeof(test_patterns) / sizeof(test_patterns[0]); i++) {
    sdram_buffer = os_mem_alloc(1024);
    if (!sdram_buffer) {
      printk("SDRAM2 malloc fail\n");
      return -EIO;
    }

    printk("SDRAM2 endereco: %p\n", sdram_buffer);
    strcpy(sdram_buffer, test_patterns[i]);

    if (strcmp(sdram_buffer, test_patterns[i]) != 0) {
      printk("Erro no padrao %d!\n", i);
      printk("  Esperado: %s\n", test_patterns[i]);
      printk("  Lido    : %s\n", sdram_buffer);
      os_mem_free(sdram_buffer);
      return -EIO;
    }
    os_mem_free(sdram_buffer);
  }

  printk("SDRAM2: testados com sucesso!\n");
  return 0;
}
