#ifndef __STRING_FORMAT_H_
#define __STRING_FORMAT_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

#include "access.h"
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>

// Tamanho máximo fixo para a string
#define MAX_FORMATTER_SIZE 32

// Estrutura para armazenar o estado da string de número inteiro
typedef struct int_formatter_t {
  char value[MAX_FORMATTER_SIZE]; // Buffer fixo de 32 bytes
  int total_digits;             // Número total de dígitos
  bool fill_with_zeros; // Se deve completar com zeros à esquerda
  bool accept_negative;         // Se aceita valores negativos
  bool is_negative;             // Indica se o valor atual é negativo
  int current_position;         // Posição atual na string
} int_formatter_t;

// Estrutura para armazenar o estado da string de número com ponto decimal
typedef struct float_formatter_t {
  char value[MAX_FORMATTER_SIZE];    // Buffer fixo de 32 bytes
  int total_digits;                // Número total de dígitos
  bool fill_with_zeros; // Se deve completar com zeros à esquerda
  bool accept_negative;            // Se aceita valores negativos
  bool is_negative;                // Indica se o valor atual é negativo
  int current_position;            // Posição atual na string
  bool has_decimal_point;          // Indica se já tem ponto decimal
  int decimal_point_position;      // Posição do ponto decimal na string
  int precision;     // Número fixo de dígitos após a vírgula (0 = posição variável)
} float_formatter_t;

// Funções para manipulação de números inteiros
void int_fmt_init(int_formatter_t* handler, bool fill_with_zeros, bool accept_negative, int total_digits);
bool int_fmt_add_digit(int_formatter_t* handler, char digit);
void int_fmt_clean(int_formatter_t* handler);
bool int_fmt_remove_digit(int_formatter_t* handler);
const char* int_fmt_get_string(int_formatter_t* handler);
int int_fmt_get_value(int_formatter_t* handler);
bool int_fmt_set_negative(int_formatter_t* handler, bool is_negative);
bool int_fmt_toggle_sign(int_formatter_t* handler);

// Funções para manipulação de números com ponto decimal
void float_fmt_init(float_formatter_t* handler, bool fill_with_zeros, int precision, bool accept_negative, int total_digits);
bool float_fmt_add_char(float_formatter_t* handler, char character);
void float_fmt_clean(float_formatter_t* handler);
bool float_fmt_remove_char(float_formatter_t* handler);
const char* float_fmt_get_string(float_formatter_t* handler);
float float_fmt_get_value(float_formatter_t* handler);
bool float_fmt_set_negative(float_formatter_t* handler, bool is_negative);
bool float_fmt_toggle_sign(float_formatter_t* handler);

// Função de teste
void formatter_test(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __STRING_FORMAT_H_ */