#include "string_format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Inicializa a estrutura do formatador de inteiros
void int_fmt_init(int_formatter_t *handler, bool fill_with_zeros,
                  bool accept_negative, int total_digits) {
  if (!handler ||
      total_digits >=
          MAX_FORMATTER_SIZE -
              1) { // -1 para deixar espaço para o sinal, se necessário
    return;        // Handler inválido ou total_digits muito grande
  }

  memset(handler->value, 0, sizeof(handler->value));
  handler->total_digits = total_digits;
  handler->fill_with_zeros = fill_with_zeros;
  handler->accept_negative = accept_negative;
  handler->is_negative = false;
  handler->current_position = 0;

  // Inicializa a string com zeros ou espaços, dependendo da configuração
  for (int i = 0; i < total_digits; i++) {
    handler->value[i] = fill_with_zeros ? '0' : ' ';
  }
  handler->value[total_digits] = '\0';
}

// Adiciona um dígito à string
bool int_fmt_add_digit(int_formatter_t *handler, char digit) {
  if (!handler || handler->current_position >= handler->total_digits) {
    return false; // Handler inválido ou string cheia
  }

  // Trata o sinal negativo
  if (digit == '-') {
    if (handler->accept_negative && handler->current_position == 0) {
      handler->is_negative = !handler->is_negative; // Permite alternar o sinal
      return true;
    }
    return false; // Não aceita sinal negativo ou posição inválida
  }

  // Verifica se o caractere é um dígito válido
  if (digit < '0' || digit > '9') {
    return false; // Caractere inválido
  }

  // Desloca todos os caracteres para a esquerda
  for (int i = 0; i < handler->total_digits - 1; i++) {
    handler->value[i] = handler->value[i + 1];
  }

  // Adiciona o novo caractere ao final
  handler->value[handler->total_digits - 1] = digit;

  // Incrementa a posição atual se ainda não estiver cheia
  if (handler->current_position < handler->total_digits) {
    handler->current_position++;
  }

  return true;
}

// Limpa a string para o estado inicial
void int_fmt_clean(int_formatter_t *handler) {
  if (!handler) {
    return;
  }

  handler->is_negative = false;
  handler->current_position = 0;

  // Reinicializa a string com zeros ou espaços, dependendo da configuração
  for (int i = 0; i < handler->total_digits; i++) {
    handler->value[i] = handler->fill_with_zeros ? '0' : ' ';
  }
  handler->value[handler->total_digits] = '\0';
}

// Remove o último dígito adicionado
bool int_fmt_remove_digit(int_formatter_t *handler) {
  if (!handler || handler->current_position <= 0) {
    return false; // Handler inválido ou string vazia
  }

  // Desloca todos os caracteres para a direita
  for (int i = handler->total_digits - 1; i > 0; i--) {
    handler->value[i] = handler->value[i - 1];
  }

  // Adiciona um zero ou espaço no início, dependendo da configuração
  handler->value[0] = handler->fill_with_zeros ? '0' : ' ';

  // Decrementa a posição atual
  handler->current_position--;

  return true;
}

// Função para obter o valor atual como string (incluindo o sinal, se
// necessário)
const char *int_fmt_get_string(int_formatter_t *handler) {
  static char result[MAX_FORMATTER_SIZE]; // Buffer estático para o resultado

  if (!handler) {
    return NULL;
  }

  // Se não for usar zeros à esquerda, precisamos remover espaços iniciais
  if (!handler->fill_with_zeros) {
    // Encontra o primeiro dígito não-espaço
    int start_pos = 0;
    while (start_pos < handler->total_digits &&
           handler->value[start_pos] == ' ') {
      start_pos++;
    }

    // Se só tem espaços, retorna "0" ou "0" com sinal
    if (start_pos == handler->total_digits) {
      if (handler->is_negative && handler->accept_negative) {
        strcpy(result, "-0");
      } else {
        strcpy(result, "0");
      }
      return result;
    }

    // Copia a parte significativa (sem espaços iniciais)
    if (handler->is_negative && handler->accept_negative) {
      result[0] = '-';
      strcpy(result + 1, handler->value + start_pos);
    } else {
      strcpy(result, handler->value + start_pos);
    }
  } else {
    // Com zeros à esquerda, simplesmente copia toda a string
    if (handler->is_negative && handler->accept_negative) {
      result[0] = '-';
      strcpy(result + 1, handler->value);
    } else {
      strcpy(result, handler->value);
    }
  }

  return result;
}

// Função para obter o valor como número (convertendo a string para int)
int int_fmt_get_value(int_formatter_t *handler) {
  if (!handler) {
    return 0;
  }

  int value = 0;

  // Converte a string para int, ignorando espaços iniciais
  for (int i = 0; i < handler->total_digits; i++) {
    if (handler->value[i] >= '0' && handler->value[i] <= '9') {
      value = value * 10 + (handler->value[i] - '0');
    }
  }

  // Aplica o sinal, se necessário
  if (handler->is_negative) {
    value = -value;
  }

  return value;
}

// Define o valor como negativo ou positivo
bool int_fmt_set_negative(int_formatter_t *handler, bool is_negative) {
  if (!handler || !handler->accept_negative) {
    return false;
  }

  handler->is_negative = is_negative;
  return true;
}

// Inverte o sinal atual
bool int_fmt_toggle_sign(int_formatter_t *handler) {
  if (!handler || !handler->accept_negative) {
    return false;
  }

  handler->is_negative = !handler->is_negative;
  return true;
}

//=================

// Inicializa a estrutura do formatador de números com ponto decimal
void float_fmt_init(float_formatter_t *handler, bool fill_with_zeros,
                    int precision, bool accept_negative, int total_digits) {
  if (!handler ||
      total_digits >=
          MAX_FORMATTER_SIZE -
              2) { // -2 para deixar espaço para o sinal e ponto decimal
    return;        // Handler inválido ou total_digits muito grande
  }

  memset(handler->value, 0, sizeof(handler->value));
  handler->total_digits = total_digits;
  handler->fill_with_zeros = fill_with_zeros;
  handler->accept_negative = accept_negative;
  handler->is_negative = false;
  handler->current_position = 0;
  handler->has_decimal_point = false;
  handler->decimal_point_position = -1;
  handler->precision = precision;

  // Inicializa a string com zeros ou espaços, dependendo da configuração
  for (int i = 0; i < total_digits; i++) {
    handler->value[i] = fill_with_zeros ? '0' : ' ';
  }
  handler->value[total_digits] = '\0';

  // Se tiver número fixo de dígitos após a vírgula, já posiciona o ponto
  // decimal
  if (precision > 0 && precision < total_digits) {
    int decimal_position = total_digits - precision - 1;
    handler->value[decimal_position] = '.';
    handler->has_decimal_point = true;
    handler->decimal_point_position = decimal_position;
  }
}

// Adiciona um caractere à string do formatador de ponto flutuante
bool float_fmt_add_char(float_formatter_t *handler, char character) {
  if (!handler || handler->current_position >= handler->total_digits) {
    return false; // Handler inválido ou string cheia
  }

  // Trata o sinal negativo
  if (character == '-') {
    if (handler->accept_negative && handler->current_position == 0) {
      handler->is_negative = !handler->is_negative; // Permite alternar o sinal
      return true;
    }
    return false; // Não aceita sinal negativo ou posição inválida
  }

  // Trata o ponto decimal
  if (character == '.' || character == ',') {
    // Se números_after_the_comma for zero, pode colocar o ponto em qualquer
    // posição
    if (handler->precision == 0 && !handler->has_decimal_point) {
      // Encontra a posição atual efetiva para inserir o ponto
      int effective_position = 0;
      for (int i = 0; i < handler->total_digits; i++) {
        if (handler->value[i] >= '0' && handler->value[i] <= '9') {
          effective_position = i;
          break;
        }
      }

      // Desloca todos os caracteres para a esquerda
      for (int i = 0; i < handler->total_digits - 1; i++) {
        handler->value[i] = handler->value[i + 1];
      }

      // Adiciona o ponto decimal
      handler->value[handler->total_digits - 1] = '.';
      handler->has_decimal_point = true;
      handler->decimal_point_position = handler->total_digits - 1;

      // Incrementa a posição atual
      if (handler->current_position < handler->total_digits) {
        handler->current_position++;
      }

      return true;
    }
    return false; // Já tem ponto decimal ou não permite ponto em posição
                  // variável
  }

  // Verifica se o caractere é um dígito válido
  if (character < '0' || character > '9') {
    return false; // Caractere inválido
  }

  // Caso especial: se tiver número fixo de dígitos após a vírgula
  if (handler->precision > 0) {
    // Não pode sobrescrever o ponto decimal
    if (handler->decimal_point_position >= 0) {
      // Desloca todos os caracteres para a esquerda, preservando o ponto
      // decimal
      char prev = character;
      char temp;
      for (int i = handler->total_digits - 1; i >= 0; i--) {
        if (i == handler->decimal_point_position) {
          continue; // Pula o ponto decimal
        }
        temp = handler->value[i];
        handler->value[i] = prev;
        prev = temp;
      }
    } else {
      // Se não tiver ponto decimal (caso improvável com precision > 0)
      // Comportamento padrão
      for (int i = 0; i < handler->total_digits - 1; i++) {
        handler->value[i] = handler->value[i + 1];
      }
      handler->value[handler->total_digits - 1] = character;
    }
  } else {
    // Comportamento padrão
    for (int i = 0; i < handler->total_digits - 1; i++) {
      handler->value[i] = handler->value[i + 1];
    }
    handler->value[handler->total_digits - 1] = character;
  }

  // Incrementa a posição atual se ainda não estiver cheia
  if (handler->current_position < handler->total_digits) {
    handler->current_position++;
  }

  return true;
}

// Limpa a string para o estado inicial
void float_fmt_clean(float_formatter_t *handler) {
  if (!handler) {
    return;
  }

  handler->is_negative = false;
  handler->current_position = 0;
  handler->has_decimal_point = false;
  handler->decimal_point_position = -1;

  // Reinicializa a string com zeros ou espaços, dependendo da configuração
  for (int i = 0; i < handler->total_digits; i++) {
    handler->value[i] = handler->fill_with_zeros ? '0' : ' ';
  }
  handler->value[handler->total_digits] = '\0';

  // Se tiver número fixo de dígitos após a vírgula, já posiciona o ponto
  // decimal
  if (handler->precision > 0 && handler->precision < handler->total_digits) {
    int decimal_position = handler->total_digits - handler->precision - 1;
    handler->value[decimal_position] = '.';
    handler->has_decimal_point = true;
    handler->decimal_point_position = decimal_position;
  }
}

// Remove o último caractere adicionado
bool float_fmt_remove_char(float_formatter_t *handler) {
  if (!handler || handler->current_position <= 0) {
    return false; // Handler inválido ou string vazia
  }

  // Caso especial: se tiver número fixo de dígitos após a vírgula
  if (handler->precision > 0) {
    // Não pode remover o ponto decimal
    if (handler->decimal_point_position >= 0) {
      // Desloca todos os caracteres para a direita, preservando o ponto decimal
      char prev = handler->fill_with_zeros ? '0' : ' ';
      char temp;
      for (int i = 0; i < handler->total_digits; i++) {
        if (i == handler->decimal_point_position) {
          continue; // Pula o ponto decimal
        }
        temp = handler->value[i];
        handler->value[i] = prev;
        prev = temp;
      }
    } else {
      // Comportamento padrão (improvável com precision > 0)
      for (int i = handler->total_digits - 1; i > 0; i--) {
        handler->value[i] = handler->value[i - 1];
      }
      handler->value[0] = handler->fill_with_zeros ? '0' : ' ';
    }
  } else {
    // Caso especial: se estiver removendo o ponto decimal
    if (handler->has_decimal_point &&
        handler->value[handler->total_digits - 1] == '.') {
      handler->has_decimal_point = false;
      handler->decimal_point_position = -1;
    }

    // Comportamento padrão
    for (int i = handler->total_digits - 1; i > 0; i--) {
      handler->value[i] = handler->value[i - 1];
    }
    handler->value[0] = handler->fill_with_zeros ? '0' : ' ';
  }

  // Decrementa a posição atual
  handler->current_position--;

  return true;
}

// Função para obter o valor atual como string (incluindo o sinal, se
// necessário)
const char *float_fmt_get_string(float_formatter_t *handler) {
  static char result[MAX_FORMATTER_SIZE]; // Buffer estático para o resultado

  if (!handler) {
    return NULL;
  }

  // Se não for usar zeros à esquerda, precisamos remover espaços iniciais
  if (!handler->fill_with_zeros) {
    // Encontra o primeiro dígito não-espaço ou o ponto decimal
    int start_pos = 0;
    while (start_pos < handler->total_digits &&
           handler->value[start_pos] == ' ' &&
           handler->value[start_pos] != '.') {
      start_pos++;
    }

    // Se só tem espaços, retorna "0" ou "-0"
    if (start_pos == handler->total_digits) {
      if (handler->is_negative && handler->accept_negative) {
        strcpy(result, "-0");
      } else {
        strcpy(result, "0");
      }
      return result;
    }

    // Se o primeiro caractere não-espaço for o ponto decimal, adiciona um zero
    // antes
    if (handler->value[start_pos] == '.') {
      if (handler->is_negative && handler->accept_negative) {
        strcpy(result, "-0");
        strcat(result, handler->value + start_pos);
      } else {
        strcpy(result, "0");
        strcat(result, handler->value + start_pos);
      }
    } else {
      // Copia a parte significativa (sem espaços iniciais)
      if (handler->is_negative && handler->accept_negative) {
        result[0] = '-';
        strcpy(result + 1, handler->value + start_pos);
      } else {
        strcpy(result, handler->value + start_pos);
      }
    }
  } else {
    // Com zeros à esquerda, simplesmente copia toda a string
    if (handler->is_negative && handler->accept_negative) {
      result[0] = '-';
      strcpy(result + 1, handler->value);
    } else {
      strcpy(result, handler->value);
    }
  }

  return result;
}

// Função para obter o valor como número float (convertendo a string para float)
float float_fmt_get_value(float_formatter_t *handler) {
  if (!handler) {
    return 0.0f;
  }

  float value = 0.0f;
  float decimal_factor = 0.1f;
  bool after_decimal = false;

  // Converte a string para float
  for (int i = 0; i < handler->total_digits; i++) {
    if (handler->value[i] == '.') {
      after_decimal = true;
      continue;
    }

    if (handler->value[i] >= '0' && handler->value[i] <= '9') {
      if (!after_decimal) {
        value = value * 10.0f + (handler->value[i] - '0');
      } else {
        value += (handler->value[i] - '0') * decimal_factor;
        decimal_factor *= 0.1f;
      }
    }
  }

  // Aplica o sinal, se necessário
  if (handler->is_negative) {
    value = -value;
  }

  return value;
}

// Define o valor como negativo ou positivo
bool float_fmt_set_negative(float_formatter_t *handler, bool is_negative) {
  if (!handler || !handler->accept_negative) {
    return false;
  }

  handler->is_negative = is_negative;
  return true;
}

// Inverte o sinal atual
bool float_fmt_toggle_sign(float_formatter_t *handler) {
  if (!handler || !handler->accept_negative) {
    return false;
  }

  handler->is_negative = !handler->is_negative;
  return true;
}

#include <zephyr/kernel.h>

void formatter_test(void) {
  // Declara as estruturas
  int_formatter_t int_with_zeros;
  int_formatter_t int_without_zeros;

  // Inicializa: com zeros à esquerda
  int_fmt_init(&int_with_zeros, true, true, 5);
  // Inicializa: sem zeros à esquerda
  int_fmt_init(&int_without_zeros, false, true, 5);

  printk("Com zeros - Após inicialização: %s\n",
         int_fmt_get_string(&int_with_zeros));
  printk("Sem zeros - Após inicialização: %s\n",
         int_fmt_get_string(&int_without_zeros));

  // Adiciona caracteres às duas estruturas
  int_fmt_add_digit(&int_with_zeros, '1');
  int_fmt_add_digit(&int_without_zeros, '1');

  printk("Com zeros - Após adicionar 1: %s\n",
         int_fmt_get_string(&int_with_zeros));
  printk("Sem zeros - Após adicionar 1: %s\n",
         int_fmt_get_string(&int_without_zeros));

  int_fmt_add_digit(&int_with_zeros, '2');
  int_fmt_add_digit(&int_without_zeros, '2');

  printk("Com zeros - Após adicionar 2: %s\n",
         int_fmt_get_string(&int_with_zeros));
  printk("Sem zeros - Após adicionar 2: %s\n",
         int_fmt_get_string(&int_without_zeros));

  // Torna ambos negativos
  int_fmt_set_negative(&int_with_zeros, true);
  int_fmt_set_negative(&int_without_zeros, true);

  printk("Com zeros - Após tornar negativo: %s\n",
         int_fmt_get_string(&int_with_zeros));
  printk("Sem zeros - Após tornar negativo: %s\n",
         int_fmt_get_string(&int_without_zeros));

  // Remove um caractere
  int_fmt_remove_digit(&int_with_zeros);
  int_fmt_remove_digit(&int_without_zeros);

  printk("Com zeros - Após remover um caractere: %s\n",
         int_fmt_get_string(&int_with_zeros));
  printk("Sem zeros - Após remover um caractere: %s\n",
         int_fmt_get_string(&int_without_zeros));

  // Limpa tudo
  int_fmt_clean(&int_with_zeros);
  int_fmt_clean(&int_without_zeros);

  printk("Com zeros - Após limpar: %s\n", int_fmt_get_string(&int_with_zeros));
  printk("Sem zeros - Após limpar: %s\n",
         int_fmt_get_string(&int_without_zeros));

  //========================

  // Exemplo 1: Número fracionário com ponto decimal em posição variável
  float_formatter_t float_variable;
  // Inicializa: aceita zeros à esquerda, ponto decimal em posição variável,
  // aceita negativo, 8 dígitos
  float_fmt_init(&float_variable, true, 0, true, 8);

  printk("Exemplo 1 - Inicialização: %s\n",
         float_fmt_get_string(&float_variable));

  float_fmt_add_char(&float_variable, '1');
  printk("Exemplo 1 - Após adicionar 1: %s\n",
         float_fmt_get_string(&float_variable));

  float_fmt_add_char(&float_variable, '2');
  printk("Exemplo 1 - Após adicionar 2: %s\n",
         float_fmt_get_string(&float_variable));

  float_fmt_add_char(&float_variable, '.'); // Adiciona ponto decimal
  printk("Exemplo 1 - Após adicionar ponto decimal: %s\n",
         float_fmt_get_string(&float_variable));

  float_fmt_add_char(&float_variable, '3');
  printk("Exemplo 1 - Após adicionar 3: %s\n",
         float_fmt_get_string(&float_variable));

  float_fmt_add_char(&float_variable, '4');
  printk("Exemplo 1 - Após adicionar 4: %s\n",
         float_fmt_get_string(&float_variable));

  float_fmt_set_negative(&float_variable, true);
  printk("Exemplo 1 - Após tornar negativo: %s\n",
         float_fmt_get_string(&float_variable));

  float valor1 = float_fmt_get_value(&float_variable);
  printk("Exemplo 1 - Valor como float: %.2f\n", valor1);

  // Exemplo 2: Número fracionário com 2 casas decimais fixas
  float_formatter_t float_fixed;
  // Inicializa: aceita zeros à esquerda, 2 casas decimais fixas, aceita
  // negativo, 8 dígitos
  float_fmt_init(&float_fixed, true, 2, true, 8);

  printk("\nExemplo 2 - Inicialização: %s\n",
         float_fmt_get_string(&float_fixed));

  float_fmt_add_char(&float_fixed, '1');
  printk("Exemplo 2 - Após adicionar 1: %s\n",
         float_fmt_get_string(&float_fixed));

  float_fmt_add_char(&float_fixed, '2');
  printk("Exemplo 2 - Após adicionar 2: %s\n",
         float_fmt_get_string(&float_fixed));

  float_fmt_add_char(&float_fixed, '3');
  printk("Exemplo 2 - Após adicionar 3: %s\n",
         float_fmt_get_string(&float_fixed));

  // Vai tentar adicionar um ponto decimal, mas será ignorado pois já temos
  // casas decimais fixas
  float_fmt_add_char(&float_fixed, '.');
  printk("Exemplo 2 - Após tentar adicionar ponto decimal (ignorado): %s\n",
         float_fmt_get_string(&float_fixed));

  float_fmt_add_char(&float_fixed, '4');
  printk("Exemplo 2 - Após adicionar 4: %s\n",
         float_fmt_get_string(&float_fixed));

  float_fmt_add_char(&float_fixed, '5');
  printk("Exemplo 2 - Após adicionar 5: %s\n",
         float_fmt_get_string(&float_fixed));

  float valor2 = float_fmt_get_value(&float_fixed);
  printk("Exemplo 2 - Valor como float: %.2f\n", valor2);

  // Limpando
  float_fmt_clean(&float_fixed);
  printk("Exemplo 2 - Após limpar: %s\n", float_fmt_get_string(&float_fixed));
}
