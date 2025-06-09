/**
 * @file    ringtone_zephyr.h
 * @brief   Biblioteca para tocar Ringtones RTTTL usando PWM com timer do Zephyr
 */

 #ifndef __RINGTONE_ZEPHYR_H_
 #define __RINGTONE_ZEPHYR_H_
 
 #include <zephyr/kernel.h>
 #include <zephyr/drivers/pwm.h>
 #include <stdint.h>
 
 /* C++ detection */
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Estrutura para configuracao do Ringtone
 typedef struct {
     const struct pwm_dt_spec *buzzer_pwm;  // Spec do PWM a ser usado
     const char *rtttl_string;              // String RTTTL
     uint8_t default_duration;              // Duracao padrao (4, 8, 16, etc)
     uint8_t default_octave;                // Oitava padrao (geralmente 5)
     uint16_t beat_value;                   // Valor de batida (quanto maior, mais rápido)
 } ringtones_config_t;
 
 // Estrutura para controle interno
 typedef struct {
     ringtones_config_t config;
     struct k_mutex mutex;                 // Mutex para protecao
     struct k_sem done_semaphore;          // Semáforo para sinalizar conclusao
     struct k_timer timer;                 // Timer para controle de tempo
     uint8_t is_playing;                   // Flag de reproducao
     uint8_t stop_request;                 // Flag para solicitar parada
     uint16_t current_index;               // Indice atual na string RTTTL
     uint32_t note_end_time;               // Tempo em que a nota atual deve terminar
     uint8_t note_processed;               // Flag indicando se a nota atual foi processada
 } ringstones_t;
 
 // Funcoes exportadas
 int ringtones_init(ringstones_t *ringstones, const struct pwm_dt_spec *buzzer_pwm,
		   const char *rtttl_string, uint8_t default_duration,
		   uint8_t default_octave, uint16_t beat_value);
		   
 int ringtones_deinit(ringstones_t *ringstones);
 int ringtones_play(ringstones_t *ringstones);
 int ringtones_stop(ringstones_t *ringstones);
 bool ringtones_is_playing(ringstones_t *ringstones);
 int ringtones_wait_done(ringstones_t *ringstones, k_timeout_t timeout);
 void ringtones_process_next_note(ringstones_t *ringstones);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* __RINGTONE_ZEPHYR_H_ */