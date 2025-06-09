#ifndef LCD_LIB_H
#define LCD_LIB_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

int lcd_init(void);
int lcd_bklight_set_percent(int percent);
int lcd_bklight_test(int test_cycles);
int lcd_lvgl_demo(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif // LCD_LIB_H
