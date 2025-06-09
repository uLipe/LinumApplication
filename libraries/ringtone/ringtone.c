/**
 * @file    ringstones.c
 * @brief   Implementacao da biblioteca para tocar Ringtones RTTTL usando PWM
 * com ISR dedicada https://adamonsoon.github.io/rtttl-play/
 * https://1j01.github.io/rtttl.js/
 * https://microblocks.fun/mbtest/NokringTunes.txt
 */

#include "ringtone.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ringtone, CONFIG_LOG_DEFAULT_LEVEL);

// Índices para as notas
#define NOTE_P 0 // Pausa
#define NOTE_A 1
#define NOTE_B 2
#define NOTE_C 3
#define NOTE_D 4
#define NOTE_E 5
#define NOTE_F 6
#define NOTE_G 7
#define NOTE_AS 8  // A#
#define NOTE_CS 9  // C#
#define NOTE_DS 10 // D#
#define NOTE_FS 11 // F#
#define NOTE_GS 12 // G#

// Tempo mínimo (em ms) para formar a duração 1/32
#define RTTTL_TIME_MIN(beat) (7500 / beat)

// Definições de frequências base para as notas (4ª oitava)
static const float NOTE_FREQUENCIES[] = {
    0,     // p (pausa)
    440.0, // a - Lá
    493.9, // b - Si
    261.6, // c - Dó
    293.7, // d - Ré
    329.6, // e - Mi
    349.2, // f - Fá
    392.0, // g - Sol
    466.2, // a# - Lá#
    277.2, // c# - Dó#
    311.1, // d# - Ré#
    370.0, // f# - Fá#
    415.3  // g# - Sol#
};

// Callback do timer
static void ringtone_timer_callback(struct k_timer *timer) {
  ringstones_t *ringstones = CONTAINER_OF(timer, ringstones_t, timer);

  bool is_playing;
  bool stop_request;
  bool note_processed;
  uint32_t note_end_time;
  uint32_t current_time;

  // Verifica parâmetros
  if (ringstones == NULL) {
    return;
  }

  // Adquire o mutex
  if (k_mutex_lock(&ringstones->mutex, K_MSEC(10)) != 0) {
    return;
  }

  // Copia variáveis locais para minimizar tempo com mutex bloqueado
  is_playing = ringstones->is_playing;
  stop_request = ringstones->stop_request;
  note_processed = ringstones->note_processed;
  note_end_time = ringstones->note_end_time;

  // Captura o tempo atual para comparação
  current_time = k_uptime_get_32();

  // Verifica se precisa processar próxima nota
  if (is_playing && (note_processed || (current_time >= note_end_time))) {
    ringstones->note_processed = 0; // Marca como não processada

    // Libera o mutex antes de processar a nota
    k_mutex_unlock(&ringstones->mutex);

    // Processa a próxima nota fora do mutex
    ringtones_process_next_note(ringstones);
  } else {
    // Libera o mutex
    k_mutex_unlock(&ringstones->mutex);
  }

  // Se houver solicitação de parada, processa isso
  if (stop_request) {
    // Desliga o PWM
    pwm_set_pulse_dt(ringstones->config.buzzer_pwm, 0);

    // Atualiza status
    if (k_mutex_lock(&ringstones->mutex, K_NO_WAIT) == 0) {
      ringstones->is_playing = 0;
      ringstones->stop_request = 0;
      k_mutex_unlock(&ringstones->mutex);
    }

    // Para o timer
    k_timer_stop(timer);

    // Sinaliza conclusão
    k_sem_give(&ringstones->done_semaphore);
  }
}

/**
 * @brief Inicializa a estrutura de ringtones e configura o timer
 */
int ringtones_init(ringstones_t *ringstones,
                   const struct pwm_dt_spec *buzzer_pwm,
                   const char *rtttl_string, uint8_t default_duration,
                   uint8_t default_octave, uint16_t beat_value) {
  // Verifica parâmetros
  if ((ringstones == NULL) || (buzzer_pwm == NULL) || (rtttl_string == NULL)) {
    return -EINVAL;
  }

  // Configura a estrutura
  ringstones->config.buzzer_pwm = buzzer_pwm;
  ringstones->config.rtttl_string = rtttl_string;
  ringstones->config.default_duration = default_duration;
  ringstones->config.default_octave = default_octave;
  ringstones->config.beat_value = beat_value;
  ringstones->current_index = 0;
  ringstones->is_playing = 0;
  ringstones->stop_request = 0;
  ringstones->note_end_time = 0;
  ringstones->note_processed = 1; // Começa como processada para iniciar nova nota

  // Inicializa mutex para proteção dos dados
  k_mutex_init(&ringstones->mutex);

  // Inicializa semáforo para sinalizar conclusão
  k_sem_init(&ringstones->done_semaphore, 0, 1);

  // Inicializa o timer
  k_timer_init(&ringstones->timer, ringtone_timer_callback, NULL);

  return 0;
}

/**
 * @brief Desaloca recursos da estrutura de ringtones
 */
int ringtones_deinit(ringstones_t *ringstones) {
  if (ringstones == NULL) {
    return -EINVAL;
  }

  // Para a reprodução
  ringtones_stop(ringstones);

  return 0;
}

/**
 * @brief Inicia a reprodução do ringtone
 */
int ringtones_play(ringstones_t *ringstones) {
  if (ringstones == NULL) {
    return -EINVAL;
  }

  // Adquire o mutex
  if (k_mutex_lock(&ringstones->mutex, K_MSEC(100)) != 0) {
    return -EAGAIN;
  }

  // Se já está tocando, não faz nada
  if (ringstones->is_playing) {
    k_mutex_unlock(&ringstones->mutex);
    return 0;
  }

  // Reinicia o índice e limpa flag de parada
  ringstones->current_index = 0;
  ringstones->stop_request = 0;
  ringstones->is_playing = 1;
  ringstones->note_processed = 1; // Para processar a primeira nota imediatamente
  ringstones->note_end_time = 0;

  // Libera o mutex
  k_mutex_unlock(&ringstones->mutex);

  // Reinicia o semáforo de conclusão
  k_sem_reset(&ringstones->done_semaphore);

  // Inicia o timer (1ms interval)
  k_timer_start(&ringstones->timer, K_MSEC(1), K_MSEC(5));

  return 0;
}

/**
 * @brief Para a reprodução do ringtone
 */
int ringtones_stop(ringstones_t *ringstones) {
  if (ringstones == NULL) {
    return -EINVAL;
  }

  // Adquire o mutex
  if (k_mutex_lock(&ringstones->mutex, K_MSEC(100)) != 0) {
    return -EAGAIN;
  }

  // Se não está tocando, não faz nada
  if (!ringstones->is_playing) {
    k_mutex_unlock(&ringstones->mutex);
    return 0;
  }

  // Solicita parada
  ringstones->stop_request = 1;
  ringstones->is_playing = 0;

  // Libera o mutex
  k_mutex_unlock(&ringstones->mutex);

  // Desliga o PWM
  pwm_set_pulse_dt(ringstones->config.buzzer_pwm, 0);

  // Para o timer
  k_timer_stop(&ringstones->timer);

  // Sinaliza conclusão
  k_sem_give(&ringstones->done_semaphore);

  return 0;
}

/**
 * @brief Verifica se o ringtone está tocando
 */
bool ringtones_is_playing(ringstones_t *ringstones) {
  bool is_playing = false;

  if (ringstones == NULL) {
    return false;
  }

  // Adquire o mutex
  if (k_mutex_lock(&ringstones->mutex, K_MSEC(10)) == 0) {
    is_playing = ringstones->is_playing;
    k_mutex_unlock(&ringstones->mutex);
  }

  return is_playing;
}

/**
 * @brief Aguarda a conclusão da reprodução
 */
int ringtones_wait_done(ringstones_t *ringstones, k_timeout_t timeout) {
  if (ringstones == NULL) {
    return -EINVAL;
  }

  // Aguarda o semáforo de conclusão
  if (k_sem_take(&ringstones->done_semaphore, timeout) != 0) {
    return -EAGAIN;
  }

  return 0;
}

/**
 * @brief Processa a próxima nota
 */
void ringtones_process_next_note(ringstones_t *ringstones) {
  const char *rtttl;
  uint16_t index;
  uint8_t duration;
  uint8_t octave;
  uint8_t note;
  uint8_t has_dot;
  uint8_t is_pause;
  uint16_t note_time;
  uint32_t current_time;
  float freq;
  uint32_t pulse_width;

  // Adquire o mutex
  if (k_mutex_lock(&ringstones->mutex, K_NO_WAIT) != 0) {
    return;
  }

  // Copia variáveis locais
  rtttl = ringstones->config.rtttl_string;
  index = ringstones->current_index;

  // Se não está tocando, sai
  if (!ringstones->is_playing) {
    k_mutex_unlock(&ringstones->mutex);
    return;
  }

  // Verifica final da música
  if (rtttl[index] == '\0') {
    // Desliga o PWM
    pwm_set_pulse_dt(ringstones->config.buzzer_pwm, 0);

    // Atualiza status
    ringstones->is_playing = 0;
    ringstones->current_index = 0;

    // Libera o mutex
    k_mutex_unlock(&ringstones->mutex);

    // Para o timer
    k_timer_stop(&ringstones->timer);

    // Sinaliza conclusão
    k_sem_give(&ringstones->done_semaphore);

    return;
  }

  // Configura valores padrão
  duration = ringstones->config.default_duration;
  octave = ringstones->config.default_octave;
  has_dot = 0;
  is_pause = 0;

  // Parse da duração
  if (rtttl[index] >= '0' && rtttl[index] <= '9') {
    if (index + 1 < strlen(rtttl) && rtttl[index + 1] >= '0' &&
        rtttl[index + 1] <= '9') {
      // Duração de dois dígitos (16, 32)
      duration = (rtttl[index] - '0') * 10 + (rtttl[index + 1] - '0');
      index += 2;
    } else {
      // Duração de um dígito (1, 2, 4, 8)
      duration = rtttl[index] - '0';
      index++;
    }
  }

  // Parse da nota
  switch (rtttl[index]) {
  case 'a':
    note = NOTE_A;
    break;
  case 'b':
    note = NOTE_B;
    break;
  case 'c':
    note = NOTE_C;
    break;
  case 'd':
    note = NOTE_D;
    break;
  case 'e':
    note = NOTE_E;
    break;
  case 'f':
    note = NOTE_F;
    break;
  case 'g':
    note = NOTE_G;
    break;
  case 'p':
    note = NOTE_P;
    is_pause = 1;
    break;
  default:
    // Nota inválida, avança para o próximo caractere
    ringstones->current_index = index + 1;
    k_mutex_unlock(&ringstones->mutex);
    return;
  }
  index++;

  // Verifica se é sustenido
  if (index < strlen(rtttl) && rtttl[index] == '#') {
    switch (note) {
    case NOTE_A:
      note = NOTE_AS;
      break;
    case NOTE_C:
      note = NOTE_CS;
      break;
    case NOTE_D:
      note = NOTE_DS;
      break;
    case NOTE_F:
      note = NOTE_FS;
      break;
    case NOTE_G:
      note = NOTE_GS;
      break;
    }
    index++;
  }

  // Verifica oitava
  if (index < strlen(rtttl) && rtttl[index] >= '4' && rtttl[index] <= '7') {
    octave = rtttl[index] - '0';
    index++;
  }

  // Verifica se tem ponto (aumenta duração em 50%)
  if (index < strlen(rtttl) && rtttl[index] == '.') {
    has_dot = 1;
    index++;
  }

  // Calcula o tempo da nota
  uint8_t n = 32 / duration;
  if (has_dot) {
    n = n + (n / 2);
  }

  note_time = n * RTTTL_TIME_MIN(ringstones->config.beat_value);

  // Atualiza o índice para o próximo caractere
  if (index < strlen(rtttl) && rtttl[index] == ',') {
    index++;
  }

  // Captura o tempo atual
  current_time = k_uptime_get_32();

  // Atualiza o tempo de término da nota
  ringstones->note_end_time = current_time + note_time;
  ringstones->current_index = index;

  // Libera o mutex
  k_mutex_unlock(&ringstones->mutex);

  // Configura a frequência da nota
  if (is_pause) {
    // Para pausas, desliga o PWM
    pwm_set_pulse_dt(ringstones->config.buzzer_pwm, 0);
  } else {
    // Calcula a frequência baseada na nota e oitava
    freq = NOTE_FREQUENCIES[note];

    // Ajusta a frequência para a oitava
    if (octave > 4) {
      // Para oitavas acima de 4, dobra a frequência para cada oitava
      for (int i = 4; i < octave; i++) {
        freq *= 2.0f;
      }
    } else if (octave < 4) {
      // Para oitavas abaixo de 4, reduz a frequência pela metade para cada
      // oitava
      for (int i = octave; i < 4; i++) {
        freq /= 2.0f;
      }
    }

    // Configura o PWM com a frequência calculada
    uint32_t period = (uint32_t)(1000000000 / freq); // Nanosegundos
    pulse_width = period / 2;                        // 50% duty cycle
    int ret = pwm_set_dt(ringstones->config.buzzer_pwm, period, pulse_width);
    if (ret) {
      LOG_ERR("Failed to set PWM: %d", ret);
    }
  }
}