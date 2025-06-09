#ifndef _LEDS_LIB_H
#define _LEDS_LIB_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

enum leds_lib_channel {
    LED_BLUE = 0,
    LED_GREEN,
    LED_RED,
    LED_WHITE,
    LED_YELLOW,
    LED_MAX,
};

int leds_lib_init(void);
int leds_lib_set(enum leds_lib_channel led, bool enable);
void leds_lib_clear(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* _LEDS_LIB_H */
