#ifndef BUZZER_LIB_H
#define BUZZER_LIB_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

int buzzer_init(void);
int buzzer_set_percent(int percent);
int buzzer_test(int test_cycles);
int buzzer_ringotne_test(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif // BUZZER_LIB_H
