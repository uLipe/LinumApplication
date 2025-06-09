#ifndef _EEPROM_LIB_H
#define _EEPROM_LIB_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int eeprom_lib_init(void);
int eeprom_lib_write(size_t offset, const void *buf, size_t buflen);
int eeprom_lib_read(size_t offset, void *buf, size_t buflen);
int eeprom_test(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* _EEPROM_LIB_H */
