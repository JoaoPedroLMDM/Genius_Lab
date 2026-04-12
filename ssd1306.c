/**
 * ssd1306.c - Driver para OLED SSD1306 via I2C
 */

// Inclui o cabeçalho correspondente com as declarações públicas do driver
#include "ssd1306.h"
// Inclui funções de manipulação de strings (memset, memcpy, strlen)
#include <string.h>
// Inclui funções utilitárias (malloc, free)
#include <stdlib.h>
// Inclui funções para manipulação de caracteres (toupper)
#include <ctype.h>

// ========== Tabela de fontes 8x8 (ASCII maiúsculas e dígitos) ==========
// Cada caractere é representado por 8 bytes, onde cada byte corresponde a uma linha de 8 pixels.
// O bit mais significativo (MSB) fica à esquerda (coluna 0).
// Índice 0 = espaço (código 0), 1-26 = letras A-Z, 27-36 = dígitos 0-9.
static const uint8_t font8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // espaço (0)
    {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // A (1)
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, // B (2)
    {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}, // C (3)
    {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}, // D (4)
    {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00}, // E (5)
    {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00}, // F (6)
    {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00}, // G (7)
    {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // H (8)
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // I (9)
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}, // J (10)
    {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}, // K (11)
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, // L (12)
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, // M (13)
    {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00}, // N (14)
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // O (15)
    {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}, // P (16)
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x0E, 0x00}, // Q (17)
    {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00}, // R (18)
    {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}, // S (19)
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T (20)
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // U (21)
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // V (22)
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W (23)
    {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}, // X (24)
    {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, // Y (25)
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}, // Z (26)
    {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00}, // 0 (27)
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // 1 (28)
    {0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E, 0x00}, // 2 (29)
    {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}, // 3 (30)
    {0x06, 0x0E, 0x1E, 0x66, 0x7F, 0x06, 0x06, 0x00}, // 4 (31)
    {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}, // 5 (32)
    {0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}, // 6 (33)
    {0x7E, 0x66, 0x0C, 0x18, 0x18, 0x18, 0x18, 0x00}, // 7 (34)
    {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}, // 8 (35)
    {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00}  // 9 (36)
};

// ========== Variáveis globais do módulo ==========
// Ponteiro para a instância I2C utilizada (i2c0 ou i2c1), definido na inicialização
static i2c_inst_t *i2c_port;
// Buffer de memória de vídeo (1 bit por pixel) no formato de páginas (8 pixels de altura por byte)
// Tamanho = (128 x 64) / 8 = 1024 bytes
static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// ========== Funções auxiliares para comunicação I2C ==========

/**
 * Envia um byte de comando para o display.
 * O SSD1306 espera o byte 0x80 seguido do byte de comando.
 */
static void write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};                         // Prefixo de comando + comando
    i2c_write_blocking(i2c_port, SSD1306_I2C_ADDR, buf, 2, false); // Envia bloqueante
}

/**
 * Envia um bloco de dados (bytes que serão escritos na GDDRAM do display).
 * O prefixo é 0x40 para indicar que são dados.
 * Aloca um buffer temporário para adicionar o prefixo aos dados.
 */
static void write_data(uint8_t *data, size_t len) {
    uint8_t *temp = malloc(len + 1);                      // Aloca espaço para prefixo + dados
    temp[0] = 0x40;                                       // Prefixo de dados
    memcpy(temp + 1, data, len);                          // Copia os dados para a posição seguinte
    i2c_write_blocking(i2c_port, SSD1306_I2C_ADDR, temp, len + 1, false);
    free(temp);                                           // Libera memória temporária
}

// ========== Inicialização do display ==========

/**
 * Sequência de inicialização padrão para o SSD1306 (128x64, interface I2C).
 * Configura modo de endereçamento horizontal, orientação, voltagem, etc.
 */
void ssd1306_init(i2c_inst_t *i2c) {
    i2c_port = i2c;                                       // Armazena a instância I2C globalmente
    // Lista de comandos de configuração (conforme datasheet)
    uint8_t cmds[] = {
        0xAE,       // Desliga o display (Display OFF)
        0x20, 0x00, // Modo de endereçamento horizontal
        0x40,       // Linha de início = 0
        0xA1,       // Segment remap: coluna 127 mapeada para SEG0 (esquerda->direita)
        0xA8, 0x3F, // Multiplex ratio: 64 (altura 64 pixels)
        0xC8,       // Direção de varredura COM: normal (cima->baixo)
        0xD3, 0x00, // Display offset = 0
        0xDA, 0x12, // Configuração de pinos COM: sequencial, remapeamento desabilitado
        0xD5, 0x80, // Clock divide ratio / frequência do oscilador
        0xD9, 0xF1, // Período de pré-carga
        0xDB, 0x30, // VCOMH deselect level
        0x81, 0xFF, // Contraste máximo (0xFF)
        0xA4,       // Display segue conteúdo da GDDRAM (não força tudo ligado)
        0xA6,       // Display normal (não invertido)
        0x8D, 0x14, // Habilita bomba de carga (necessário para alimentação interna)
        0x2E,       // Desativa rolagem (scroll)
        0xAF        // Liga o display (Display ON)
    };
    // Envia cada comando da sequência
    for (int i = 0; i < sizeof(cmds); i++)
        write_cmd(cmds[i]);
    
    // Limpa o buffer local e envia para o display (tela apagada)
    ssd1306_clear();
    ssd1306_show();
}

// ========== Gerenciamento do buffer de vídeo ==========

/**
 * Preenche todo o buffer de vídeo com zeros (pixels apagados).
 */
void ssd1306_clear(void) {
    memset(buffer, 0, sizeof(buffer));
}

/**
 * Envia o conteúdo completo do buffer para a GDDRAM do display,
 * configurando a janela de colunas (0-127) e páginas (0-7) antes da transferência.
 */
void ssd1306_show(void) {
    // Define endereço de coluna: início 0, fim 127
    write_cmd(0x21); write_cmd(0); write_cmd(127);
    // Define endereço de página: início 0, fim 7 (8 páginas para 64 linhas)
    write_cmd(0x22); write_cmd(0); write_cmd(7);
    // Envia os 1024 bytes do buffer como dados
    write_data(buffer, sizeof(buffer));
}

// ========== Desenho de primitivas (pixel, caractere) ==========

/**
 * Função interna para modificar um único pixel no buffer.
 * Verifica limites e calcula a página e o bit correspondente.
 */
static void draw_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return; // Fora dos limites
    int page = y / 8;                     // Cada página agrupa 8 linhas verticais (0-7)
    int bit = y % 8;                      // Posição do bit dentro do byte da página
    if (on)
        buffer[page * SSD1306_WIDTH + x] |= (1 << bit);   // Liga o bit
    else
        buffer[page * SSD1306_WIDTH + x] &= ~(1 << bit);  // Desliga o bit
}

/**
 * Desenha um caractere 8x8 pixels na posição (x, y).
 * Assume que y é múltiplo de 8 (alinhado às páginas) para simplificar.
 * Converte o caractere para maiúscula e busca seu índice na tabela de fontes.
 */
static void draw_char(int x, int y, char c) {
    if (x < 0 || x > SSD1306_WIDTH - 8 || y < 0 || y > SSD1306_HEIGHT - 8) return; // Não cabe
    c = toupper(c);                             // Converte para maiúscula
    int idx = 0;                                // Índice padrão = espaço (0)
    if (c >= 'A' && c <= 'Z') idx = c - 'A' + 1;       // A->1, B->2, ..., Z->26
    else if (c >= '0' && c <= '9') idx = c - '0' + 27; // 0->27, 1->28, ..., 9->36
    
    // Percorre as 8 linhas da fonte
    for (int row = 0; row < 8; row++) {
        uint8_t line = font8x8[idx][row];       // Byte da linha atual
        // Percorre as 8 colunas (bits da esquerda para a direita)
        for (int col = 0; col < 8; col++) {
            // Verifica se o bit (MSB primeiro) está ligado
            if (line & (1 << (7 - col)))        // MSB corresponde à coluna 0
                draw_pixel(x + col, y + row, true); // Liga o pixel correspondente
        }
    }
}

// ========== Função pública para desenho de strings ==========

/**
 * Desenha uma string a partir da posição (x, y), avançando 8 pixels horizontalmente
 * para cada caractere. Para se atingir o limite direito do display.
 */
void ssd1306_draw_string(int x, int y, const char *str) {
    while (*str) {                          // Enquanto não chegar ao fim da string
        draw_char(x, y, *str++);            // Desenha o caractere atual e avança ponteiro
        x += 8;                             // Próximo caractere 8 pixels à direita
        if (x > SSD1306_WIDTH - 8) break;   // Sai se não houver espaço para mais um caractere
    }
}