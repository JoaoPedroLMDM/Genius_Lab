/**
 * Biblioteca SSD1306 para BitDogLab (RP2040)
 * Adaptado do exemplo oficial do Pico SDK
 * 
 * Conexões:
 * - SDA: GPIO14
 * - SCL: GPIO15
 * - Endereço I2C: 0x3C
 */

// Início da guarda de inclusão múltipla (header guard)
// Evita que o conteúdo do arquivo seja processado mais de uma vez durante a compilação
#ifndef SSD1306_H
#define SSD1306_H

// Inclui a biblioteca padrão do Pico SDK (fornece tipos básicos e funções utilitárias)
#include "pico/stdlib.h"
// Inclui a API de hardware I2C para comunicação com o display
#include "hardware/i2c.h"

// ========== Definições das dimensões físicas do display OLED ==========
// Largura do display em pixels (128 colunas)
#define SSD1306_WIDTH   128
// Altura do display em pixels (64 linhas)
#define SSD1306_HEIGHT  64

// Endereço I2C padrão do controlador SSD1306 (7 bits, deslocamento zero)
// O macro _u() é utilizado para forçar o tipo unsigned, prevenindo warnings
#define SSD1306_I2C_ADDR _u(0x3C)

// ========== Protótipos das funções públicas do driver ==========

/**
 * Inicializa o display SSD1306 utilizando o barramento I2C especificado.
 * Deve ser chamada uma vez antes de qualquer outra operação com o display.
 * @param i2c Ponteiro para a instância I2C (i2c0 ou i2c1) já inicializada.
 */
void ssd1306_init(i2c_inst_t *i2c);

/**
 * Limpa todo o buffer de vídeo interno, apagando todos os pixels.
 * Esta função não atualiza o display imediatamente; use ssd1306_show() para isso.
 */
void ssd1306_clear(void);

/**
 * Desenha uma string (texto) no buffer de vídeo a partir da posição especificada.
 * A fonte utilizada é de tamanho fixo 8x8 pixels.
 * Caracteres são convertidos para maiúsculas automaticamente.
 * @param x Coordenada X (coluna) do canto superior esquerdo do primeiro caractere (0 a 127)
 * @param y Coordenada Y (linha) do canto superior esquerdo do primeiro caractere (0 a 63)
 * @param str Ponteiro para a string terminada em nulo a ser desenhada
 */
void ssd1306_draw_string(int x, int y, const char *str);

/**
 * Envia o conteúdo completo do buffer de vídeo para o display, atualizando a tela.
 * Deve ser chamada após qualquer operação de desenho para que as alterações sejam visíveis.
 */
void ssd1306_show(void);

// Fim da guarda de inclusão múltipla
#endif