#ifndef _SDCARCD_LIB_H
#define _SDCARCD_LIB_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int sdcard_init(void);
int sdcard_test(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* _SDCARCD_LIB_H */
