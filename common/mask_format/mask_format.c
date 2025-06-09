#include "mask_format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Tabela de configurações para cada tipo de formato
static const struct {
  const char *mask;
  char placeholder_digit;
  char placeholder_alpha;
  int max_length;
  bool dynamic_input;
} FORMAT_CONFIGS[FORMAT_TYPE_COUNT] = {
    [FORMAT_TD_ISO_8601_TYPE_1] = {"DDDD-DD-DD DD:DD:DD", '0', ' ', 19, true},
    [FORMAT_TD_ISO_8601_TYPE_2] = {"DD-DD-DDDD DD:DD:DD", '0', ' ', 19, true},
    [FORMAT_TD_ISO_8601_TYPE_3] = {"DD:DD:DD DDDD-DD-DD", '0', ' ', 19, true},
    [FORMAT_TD_ISO_8601_TYPE_4] = {"DD:DD:DD DD-DD-DDDD", '0', ' ', 19, true},
    [FORMAT_TD_DD_MM_YYYY_TYPE_1] = {"DD/DD/DDDD DD:DD:DD", '0', ' ', 19, true},
    [FORMAT_TD_YYYY_MM_DD_TYPE_2] = {"DDDD/DD/DD DD:DD:DD", '0', ' ', 19, true},
    [FORMAT_TD_DD_MM_YYYY_TYPE_3] = {"DD:DD:DD DD/DD/DDDD", '0', ' ', 19, true},
    [FORMAT_TD_YYYY_MM_DD_TYPE_4] = {"DD:DD:DD DDDD/DD/DD", '0', ' ', 19, true},
    [FORMAT_TIME_ISO_8601_TYPE_1] = {"DD:DD:DD", '0', ' ', 8, true},
    [FORMAT_TIME_ISO_8601_TYPE_2] = {"DD:DD", '0', ' ', 5, true},
    [FORMAT_DATE_ISO_8601_TYPE_1] = {"DDDD-DD-DD", '0', ' ', 10, true},
    [FORMAT_DATE_ISO_8601_TYPE_2] = {"DD-DD-DDDD", '0', ' ', 10, true},
    [FORMAT_DD_MM_YYYY_TYPE_1] = {"DD/DD/DDDD", '0', ' ', 10, true},
    [FORMAT_YYYY_MM_DD_TYPE_2] = {"DDDD/DD/DD", '0', ' ', 10, true},
    [FORMAT_IP_ADDR] = {"DDD.DDD.DDD.DDD", '0', ' ', 15, true},
    [FORMAT_MAC_ADDR] = {"EE:EE:EE:EE:EE:EE", '0', ' ', 17, true},
    [FORMAT_HEXA_TYPE_1] = {"0xEEEEEEEE", '0', ' ', 10, true},
    [FORMAT_HEXA_TYPE_2] = {"0xEEEEEEEE", '0', ' ', 10, true}};

// Função auxiliar para determinar o tipo de caractere na máscara
static mask_char_type_t get_mask_char_type(char c) {
  switch (c) {
  case 'D':
    return MASK_CHAR_TYPE_DIGIT;
  case 'E':
    return MASK_CHAR_TYPE_HEX_REQ;
  case 'e':
    return MASK_CHAR_TYPE_HEX_OPT;
  case 'S':
    return MASK_CHAR_TYPE_ALPHA;
  case 'B':
    return MASK_CHAR_TYPE_BINARY;
  default:
    return MASK_CHAR_TYPE_FIXED;
  }
}

// Função auxiliar para verificar se um caractere é válido para o tipo
// especificado
static bool is_valid_char_for_type(char c, mask_char_type_t type) {
  switch (type) {
  case MASK_CHAR_TYPE_DIGIT:
    return isdigit(c);
  case MASK_CHAR_TYPE_HEX_REQ:
  case MASK_CHAR_TYPE_HEX_OPT:
    return isxdigit(c);
  case MASK_CHAR_TYPE_ALPHA:
    return isalpha(c);
  case MASK_CHAR_TYPE_BINARY:
    return (c == '0' || c == '1');
  default:
    return true;
  }
}

// Inicializa o placeholder com base no tipo de caractere
static char get_placeholder_for_type(mask_char_type_t type,
                                     const generic_formatter_t *formatter) {
  switch (type) {
  case MASK_CHAR_TYPE_DIGIT:
  case MASK_CHAR_TYPE_HEX_REQ:
  case MASK_CHAR_TYPE_HEX_OPT:
  case MASK_CHAR_TYPE_BINARY:
    return formatter->placeholder_digit;
  case MASK_CHAR_TYPE_ALPHA:
    return formatter->placeholder_alpha;
  default:
    return ' ';
  }
}

void generic_fmt_init(generic_formatter_t *formatter, format_type_t type) {
  if (!formatter || type >= FORMAT_TYPE_COUNT)
    return;

  memset(formatter->buffer, 0, MAX_FORMATTER_SIZE);

  formatter->mask = FORMAT_CONFIGS[type].mask;
  formatter->placeholder_digit = FORMAT_CONFIGS[type].placeholder_digit;
  formatter->placeholder_alpha = FORMAT_CONFIGS[type].placeholder_alpha;
  formatter->dynamic_input = FORMAT_CONFIGS[type].dynamic_input;
  formatter->max_length = FORMAT_CONFIGS[type].max_length;
  formatter->current_pos = 0;
  formatter->is_initialized = true;
  formatter->is_complete = false;

  // Preenche o buffer inicial de acordo com a máscara
  for (int i = 0; i < formatter->max_length; i++) {
    char mask_char = formatter->mask[i];
    mask_char_type_t char_type = get_mask_char_type(mask_char);

    if (char_type == MASK_CHAR_TYPE_FIXED) {
      // Caracteres fixos são copiados diretamente
      formatter->buffer[i] = mask_char;
    } else {
      // Caracteres variáveis são preenchidos com o placeholder apropriado
      formatter->buffer[i] = get_placeholder_for_type(char_type, formatter);
    }
  }

  formatter->buffer[formatter->max_length] = '\0';
}

bool generic_fmt_add_char(generic_formatter_t *formatter, char c) {
  if (!formatter || !formatter->is_initialized) {
    return false;
  }

  // Se já estamos na posição máxima, não pode adicionar mais
  if (formatter->current_pos >= formatter->max_length) {
    return false;
  }

  // Avança até a próxima posição editável
  while (formatter->current_pos < formatter->max_length) {
    char mask_char = formatter->mask[formatter->current_pos];
    mask_char_type_t char_type = get_mask_char_type(mask_char);

    // Se é um caractere fixo, passamos para a próxima posição
    if (char_type == MASK_CHAR_TYPE_FIXED) {
      formatter->current_pos++;
      continue;
    }

    // Verifica se o caractere é válido para o tipo na máscara
    if (!is_valid_char_for_type(c, char_type)) {
      return false;
    }

    // Adiciona o caractere na posição atual
    formatter->buffer[formatter->current_pos++] = c;

    // Verifica se o preenchimento está completo
    formatter->is_complete = generic_fmt_is_complete(formatter);

    return true;
  }

  return false;
}

bool generic_fmt_set_value(generic_formatter_t *formatter, const char *value) {
  if (!formatter || !formatter->is_initialized || !value) {
    return false;
  }

  generic_fmt_reset(formatter);

  for (int i = 0; value[i] != '\0'; i++) {
    if (!generic_fmt_add_char(formatter, value[i])) {
      break;
    }
  }

  formatter->is_complete = generic_fmt_is_complete(formatter);
  return formatter->is_complete;
}

const char *generic_fmt_get_string(const generic_formatter_t *formatter) {
  if (!formatter || !formatter->is_initialized) {
    return NULL;
  }
  return formatter->buffer;
}

void generic_fmt_reset(generic_formatter_t *formatter) {
  if (!formatter || !formatter->is_initialized) {
    return;
  }

  // Reinicializa o buffer de acordo com a máscara
  for (int i = 0; i < formatter->max_length; i++) {
    char mask_char = formatter->mask[i];
    mask_char_type_t char_type = get_mask_char_type(mask_char);

    if (char_type == MASK_CHAR_TYPE_FIXED) {
      // Caracteres fixos são copiados diretamente
      formatter->buffer[i] = mask_char;
    } else {
      // Caracteres variáveis são preenchidos com o placeholder apropriado
      formatter->buffer[i] = get_placeholder_for_type(char_type, formatter);
    }
  }

  formatter->buffer[formatter->max_length] = '\0';
  formatter->current_pos = 0;
  formatter->is_complete = false;
}

bool generic_fmt_is_complete(const generic_formatter_t *formatter) {
  if (!formatter || !formatter->is_initialized) {
    return false;
  }

  // Verifica se todos os campos obrigatórios estão preenchidos
  for (int i = 0; i < formatter->max_length; i++) {
    char mask_char = formatter->mask[i];
    mask_char_type_t char_type = get_mask_char_type(mask_char);

    // Verifica se o campo é obrigatório
    bool is_required = (char_type == MASK_CHAR_TYPE_DIGIT ||
                        char_type == MASK_CHAR_TYPE_HEX_REQ ||
                        char_type == MASK_CHAR_TYPE_ALPHA ||
                        char_type == MASK_CHAR_TYPE_BINARY);

    // Se é obrigatório e ainda tem o placeholder, então não está completo
    if (is_required) {
      char expected_placeholder =
          get_placeholder_for_type(char_type, formatter);
      if (formatter->buffer[i] == expected_placeholder) {
        return false;
      }
    }
  }

  return true;
}

bool generic_fmt_is_full(const generic_formatter_t *formatter) {
  if (!formatter || !formatter->is_initialized) {
    return false;
  }

  return formatter->current_pos >= formatter->max_length;
}

#include <zephyr/kernel.h>

void mask_formatter_test(void) {
  generic_formatter_t formatter;

  // Teste 1: Data e hora ISO 8601 (tipo 1)
  generic_fmt_init(&formatter, FORMAT_TD_ISO_8601_TYPE_1);
  printk("Máscara inicial: %s\n", generic_fmt_get_string(&formatter));

  const char *datetime = "20230415123045"; // 2023-04-15 12:30:45
  for (int i = 0; datetime[i] != '\0'; i++) {
    generic_fmt_add_char(&formatter, datetime[i]);
  }

  printk("Data/Hora formatada: %s\n", generic_fmt_get_string(&formatter));
  printk("Está completo? %s\n",
         generic_fmt_is_complete(&formatter) ? "Sim" : "Não");

  // Teste 2: Endereço MAC
  generic_fmt_init(&formatter, FORMAT_MAC_ADDR);
  printk("\nMáscara inicial MAC: %s\n", generic_fmt_get_string(&formatter));

  const char *mac = "A1B2C3D4E5F6";
  for (int i = 0; mac[i] != '\0'; i++) {
    generic_fmt_add_char(&formatter, mac[i]);
  }

  printk("MAC formatado: %s\n", generic_fmt_get_string(&formatter));
  printk("Está completo? %s\n",
         generic_fmt_is_complete(&formatter) ? "Sim" : "Não");

  // Teste 3: Valor Hexadecimal
  generic_fmt_init(&formatter, FORMAT_HEXA_TYPE_1);
  printk("\nMáscara inicial Hexa: %s\n", generic_fmt_get_string(&formatter));

  const char *hex = "ABCDEF12";
  for (int i = 0; hex[i] != '\0'; i++) {
    generic_fmt_add_char(&formatter, hex[i]);
  }

  printk("Hexadecimal formatado: %s\n", generic_fmt_get_string(&formatter));
  printk("Está completo? %s\n",
         generic_fmt_is_complete(&formatter) ? "Sim" : "Não");

  // Teste 4: Endereço IP usando set_value
  generic_fmt_init(&formatter, FORMAT_IP_ADDR);
  generic_fmt_set_value(&formatter, "192168001001");

  printk("\nIP formatado: %s\n", generic_fmt_get_string(&formatter));
  printk("Está completo? %s\n",
         generic_fmt_is_complete(&formatter) ? "Sim" : "Não");
}
