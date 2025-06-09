#ifndef __MASK_FORMAT_H_
#define __MASK_FORMAT_H_

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include "access.h"
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>

// Tamanho máximo fixo para a string
#define MAX_FORMATTER_SIZE 32

typedef enum {
  FORMAT_TD_ISO_8601_TYPE_1,   // YYYY-MM-DD HH:MM:SS (DDDD-DD-DD DD:DD:DD)
  FORMAT_TD_ISO_8601_TYPE_2,   // DD-MM-YYYY HH:MM:SS (DD-DD-DDDD DD:DD:DD)
  FORMAT_TD_ISO_8601_TYPE_3,   // HH:MM:SS YYYY-MM-DD (DD:DD:DD DDDD-DD-DD)
  FORMAT_TD_ISO_8601_TYPE_4,   // HH:MM:SS DD-MM-YYYY (DD:DD:DD DD-DD-DDDD)
  FORMAT_TD_DD_MM_YYYY_TYPE_1, // DD/MM/YYYY HH:MM:SS
  FORMAT_TD_YYYY_MM_DD_TYPE_2, // YYYY/MM/DD HH:MM:SS
  FORMAT_TD_DD_MM_YYYY_TYPE_3, // HH:MM:SS DD/MM/YYYY
  FORMAT_TD_YYYY_MM_DD_TYPE_4, // HH:MM:SS YYYY/MM/DD
  FORMAT_TIME_ISO_8601_TYPE_1, // HH:MM:SS (DD:DD:DD)
  FORMAT_TIME_ISO_8601_TYPE_2, // HH:MM (DD:DD)
  FORMAT_DATE_ISO_8601_TYPE_1, // YYYY-MM-DD (DDDD-DD-DD)
  FORMAT_DATE_ISO_8601_TYPE_2, // DD-MM-YYYY (DD-DD-DDDD)
  FORMAT_DD_MM_YYYY_TYPE_1,    // DD/MM/YYYY
  FORMAT_YYYY_MM_DD_TYPE_2,    // YYYY/MM/DD
  FORMAT_IP_ADDR,              // DDD.DDD.DDD.DDD
  FORMAT_MAC_ADDR,             // EE:EE:EE:EE:EE:EE
  FORMAT_HEXA_TYPE_1,          // 0xEEEEEEEE
  FORMAT_HEXA_TYPE_2, // 0xEEEEEEEE (Duplicado intencionalmente para manter
                      // compatibilidade)
  FORMAT_TYPE_COUNT   // Marcador de quantidade
} format_type_t;

// Tipos de caracteres na máscara
typedef enum {
  MASK_CHAR_TYPE_FIXED,   // Caractere fixo (separador)
  MASK_CHAR_TYPE_DIGIT,   // D: Dígito (0-9) obrigatório
  MASK_CHAR_TYPE_HEX_REQ, // E: Valor hexadecimal (0-9, A-F) obrigatório
  MASK_CHAR_TYPE_HEX_OPT, // e: Valor hexadecimal (0-9, A-F) não obrigatório
  MASK_CHAR_TYPE_ALPHA,   // S: Caractere alfabético (A-Z, a-z) obrigatório
  MASK_CHAR_TYPE_BINARY   // B: Caractere binário (0-1) obrigatório
} mask_char_type_t;

typedef struct {
  char buffer[MAX_FORMATTER_SIZE]; // Buffer para a string sendo formatada
  const char *mask;                // Máscara de formatação
  char placeholder_digit;          // Caractere para preenchimento de números
  char placeholder_alpha;          // Caractere para preenchimento alfabético
  bool dynamic_input;              // Se aceita entrada dinâmica de caracteres
  int max_length;                  // Tamanho máximo da string formatada
  int current_pos;                 // Posição atual no buffer
  bool is_initialized;             // Se o formatador foi inicializado
  bool is_complete; // Se todos os campos obrigatórios foram preenchidos
} generic_formatter_t;

// Inicializa o formatador com uma configuração específica
void generic_fmt_init(generic_formatter_t *formatter, format_type_t type);

// Adiciona um caractere à string sendo formatada (tratamento dinâmico)
bool generic_fmt_add_char(generic_formatter_t *formatter, char c);

// Define o valor completo de uma vez (para casos não dinâmicos)
bool generic_fmt_set_value(generic_formatter_t *formatter, const char *value);

// Retorna a string formatada atual
const char *generic_fmt_get_string(const generic_formatter_t *formatter);

// Reinicia o formatador
void generic_fmt_reset(generic_formatter_t *formatter);

// Verifica se o formatador está completo (todos os campos obrigatórios estão
// preenchidos)
bool generic_fmt_is_complete(const generic_formatter_t *formatter);

// Verifica se o formatador está cheio (atingiu o tamanho máximo)
bool generic_fmt_is_full(const generic_formatter_t *formatter);

// Função de teste
void mask_formatter_test(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __STRING_FORMAT_H_ */
