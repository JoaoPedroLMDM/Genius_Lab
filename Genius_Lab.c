/**
 * Projeto Final EmbarcaTech: "Genius Lab"
 * Jogo da Memória - BitDogLab (RP2040) com Wi-Fi e MQTT
 *
 * Funcionalidades:
 * - 3 níveis de dificuldade (Fácil, Normal, Difícil)
 * - DMA para matriz WS2812B (bônus)
 * - High Score salvo na flash
 * - Conectividade Wi-Fi e publicação MQTT de recordes
 * - Melodias: abertura (Mario), início (Korobeiniki), game over (Sad Trombone)
 * - Dois buzzers alternados (duty cycle 12.5%, sem aquecimento)
 * - Interrupção (IRQ) no botão do joystick
 * - Comunicação UART (printf)
 */

// --- Inclusão de bibliotecas padrão e do SDK ---
#include <stdio.h>              // Entrada/saída padrão (printf, sprintf)
#include <stdlib.h>             // Funções utilitárias (rand, srand)
#include <string.h>             // Manipulação de strings (strlen)
#include "pico/stdlib.h"        // Biblioteca padrão do Pico (GPIO, timer, etc.)
#include "pico/cyw43_arch.h"    // Suporte ao chip Wi-Fi CYW43439 (BitDogLab)
#include "hardware/gpio.h"      // Controle de GPIO
#include "hardware/adc.h"       // Leitura do conversor analógico-digital (joystick)
#include "hardware/i2c.h"       // Comunicação I2C (display OLED)
#include "hardware/pwm.h"       // Geração de PWM (buzzers)
#include "hardware/timer.h"     // Funções de temporização
#include "hardware/pio.h"       // Programação de blocos PIO (WS2812)
#include "hardware/dma.h"       // Acesso direto à memória (DMA) para LEDs
#include "hardware/clocks.h"    // Configuração de clocks do sistema
#include "hardware/flash.h"     // Escrita/leitura na memória flash (high score)
#include "hardware/sync.h"      // Controle de interrupções (proteger escrita na flash)
#include "lwip/apps/mqtt.h"     // Cliente MQTT do lwIP
#include "lwip/dns.h"           // Resolução de DNS para o broker MQTT
#include "ws2812.pio.h"         // Cabeçalho gerado do programa PIO para WS2812
#include "ssd1306.h"            // Driver do display OLED SSD1306

// ========== CONFIGURAÇÕES DE REDE ==========
#define WIFI_SSID       "REGINALDO"                 // Nome da rede Wi-Fi
#define WIFI_PASSWORD   "13031974"                  // Senha da rede Wi-Fi
#define MQTT_BROKER     "broker.hivemq.com"         // Endereço do broker MQTT público
#define MQTT_TOPIC      "embarcatech/geniuslab/score" // Tópico MQTT para publicação do recorde

// ========== DEFINIÇÕES DE HARDWARE ==========
#define WS2812_PIN          7       // Pino de dados da matriz de LEDs WS2812
#define NUM_PIXELS          25      // Número total de LEDs (matriz 5x5)
#define OLED_I2C            i2c1    // Barramento I2C usado para o OLED
#define OLED_SDA            14      // Pino SDA do I2C
#define OLED_SCL            15      // Pino SCL do I2C
#define BUTTON_A            5       // Pino do botão A
#define BUTTON_B            6       // Pino do botão B
#define JOY_BTN             22      // Pino do botão do joystick (pressionar)
#define JOY_X               26      // Eixo X do joystick (ADC0)
#define JOY_Y               27      // Eixo Y do joystick (ADC1)
#define BUZZER_LEFT_PIN     21      // Pino do buzzer esquerdo (PWM)
#define BUZZER_RIGHT_PIN    10      // Pino do buzzer direito (PWM)

// ========== PARÂMETROS DO JOGO ==========
#define MAX_SEQUENCE     50      // Tamanho máximo da sequência (nível máximo)
#define NUM_COLORS       4       // Número de cores/quadrantes

// Cores dos LEDs (formato RGB 24 bits)
const uint32_t COLORS[NUM_COLORS] = { 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF0000 };
// Frequências dos tons correspondentes a cada cor (C4, E4, G4, B4)
const uint16_t TONES[NUM_COLORS]  = { 262, 330, 392, 494 };
// Tons especiais para acerto e erro
#define TONE_SUCCESS 800      // Frequência para som de sucesso (passagem de nível)
#define TONE_FAIL    200      // Frequência para som de erro (game over)

// ========== CONFIGURAÇÃO DE DIFICULDADE ==========
typedef enum { DIFF_EASY, DIFF_NORMAL, DIFF_HARD } difficulty_t; // Enumeração dos níveis

// Estrutura que agrupa as configurações de cada dificuldade
typedef struct {
    const char *name;                     // Nome exibido no OLED
    const uint8_t *quadrants[NUM_COLORS]; // Ponteiros para arrays com índices dos LEDs de cada cor
    uint8_t led_count[NUM_COLORS];        // Quantos LEDs acendem por cor
    uint32_t tone_duration_ms;            // Duração de cada tom (ms)
    uint32_t pause_ms;                    // Pausa entre tons (ms)
} difficulty_config_t;

// Mapeamento de quadrantes para dificuldades FÁCIL e NORMAL (4 LEDs por cor)
const uint8_t easy_quadrants[NUM_COLORS][4] = {
    {0,1,5,6},      // Verde
    {2,3,7,8},      // Azul
    {10,11,15,16},  // Amarelo
    {12,13,17,18}   // Vermelho
};

// Mapeamento para dificuldade DIFÍCIL (apenas 1 LED central por cor)
const uint8_t hard_green[]  = {6};   // LED central superior esquerdo
const uint8_t hard_blue[]   = {8};   // LED central superior direito
const uint8_t hard_yellow[] = {16};  // LED central inferior esquerdo
const uint8_t hard_red[]    = {18};  // LED central inferior direito

// Tabela de configurações para cada nível de dificuldade
const difficulty_config_t difficulties[] = {
    [DIFF_EASY]   = { "FACIL",  {easy_quadrants[0], easy_quadrants[1], easy_quadrants[2], easy_quadrants[3]}, {4,4,4,4}, 600, 300 },
    [DIFF_NORMAL] = { "NORMAL", {easy_quadrants[0], easy_quadrants[1], easy_quadrants[2], easy_quadrants[3]}, {4,4,4,4}, 400, 200 },
    [DIFF_HARD]   = { "DIFICIL",{hard_green, hard_blue, hard_yellow, hard_red},                               {1,1,1,1}, 200, 100 }
};

// ========== HIGH SCORE NA MEMÓRIA FLASH ==========
#define FLASH_TARGET_OFFSET (256 * 1024)                    // Offset de 256 KB na flash
#define FLASH_HIGH_SCORE_ADDR (XIP_BASE + FLASH_TARGET_OFFSET) // Endereço mapeado em tempo de execução

typedef struct { uint32_t magic; uint32_t high_score; } flash_data_t; // Estrutura gravada na flash
#define FLASH_MAGIC 0xDEADBEEF   // Valor mágico para validar dados

// ========== MELODIAS ==========
// Abertura (Super Mario)
const uint16_t startup_notes[] = { 659,659,0,659,0,523,659,0,784,0,0,0,392 };
const uint32_t startup_durs[]  = { 150,150,150,150,150,150,150,150,300,300,150,150,300 };
#define STARTUP_LEN (sizeof(startup_notes)/sizeof(startup_notes[0]))

// Início de jogo (Korobeiniki / Tetris)
const uint16_t start_notes[]   = { 659,494,523,587,659,587,523,494,440,440,523,659,587,523,494 };
const uint32_t start_durs[]    = { 200,200,200,200,200,200,200,200,200,200,200,200,200,200,400 };
#define START_LEN (sizeof(start_notes)/sizeof(start_notes[0]))

// Game Over (Sad Trombone)
const uint16_t over_notes[]    = { 523,494,440,392,349,330,294,262,247,233,220 };
const uint32_t over_durs[]     = { 120,120,120,120,120,120,120,120,120,120,400 };
#define OVER_LEN (sizeof(over_notes)/sizeof(over_notes[0]))

// ========== ESTADOS DO JOGO ==========
typedef enum { MENU, SEQUENCE_SHOW, PLAYER_INPUT, GAME_OVER } state_t;
volatile state_t game_state = MENU;   // Estado atual (volatile pois é alterado na IRQ)
volatile bool irq_flag = false;       // Flag sinalizando interrupção do botão do joystick
volatile uint64_t last_irq = 0;       // Timestamp da última IRQ (debounce)

uint8_t sequence[MAX_SEQUENCE];   // Array com a sequência de cores (índices 0..3)
uint8_t seq_len = 0, player_step = 0; // Comprimento atual e passo do jogador
uint32_t high_score = 0;          // Recorde armazenado
difficulty_t current_diff = DIFF_NORMAL; // Dificuldade selecionada (padrão: normal)

PIO pio = pio0;   // Usa bloco PIO 0
uint sm = 0;      // State machine 0
int dma_chan;     // Canal DMA alocado
static int current_buzzer = 0; // Alterna entre buzzers (0 = esquerdo, 1 = direito)

// Variáveis globais para Wi-Fi e MQTT
static bool wifi_connected = false;      // Indica se o Wi-Fi está conectado
static mqtt_client_t *mqtt_client = NULL; // Cliente MQTT
static char mqtt_payload[64];            // Buffer para mensagem JSON enviada ao broker
static ip_addr_t mqtt_broker_ip;         // IP do broker MQTT resolvido via DNS

// ========== PROTÓTIPOS DAS FUNÇÕES ==========
void init_hardware(void);
void buzzer_init(void);
void buzzer_play_tone(uint16_t freq, uint32_t ms);
void play_tone(uint16_t freq, uint32_t ms);
void gpio_callback(uint gpio, uint32_t events);
void set_led_color_dma(uint32_t color, const uint8_t *idx, uint8_t cnt);
void clear_leds(void);
void play_melody(const uint16_t *notes, const uint32_t *durs, size_t len);
void update_oled(const char *l1, const char *l2, const char *l3);
void display_high_score(void);
void generate_step(void);
void play_sequence(void);
uint8_t read_input(void);
bool check_input(uint8_t sel);
void game_over(void);
void load_high_score(void);
void save_high_score(uint32_t new_score);
void wifi_init_and_connect(void);
void mqtt_publish_score(uint32_t score);

// ========== CALLBACKS MQTT ==========
// Chamada quando a conexão MQTT é estabelecida ou falha
static void mqtt_connection_cb(mqtt_client_t *c, void *arg, mqtt_connection_status_t s) {
    printf("MQTT %s\n", s == MQTT_CONNECT_ACCEPTED ? "conectado" : "erro");
}
// Chamada quando uma publicação MQTT é concluída
static void mqtt_publish_cb(void *arg, err_t err) {
    printf("Publicacao MQTT %s\n", err == ERR_OK ? "OK" : "falhou");
}

// ========== RESOLUÇÃO DNS DO BROKER MQTT ==========
static bool resolve_mqtt_broker(void) {
    printf("Resolvendo DNS para %s...\n", MQTT_BROKER);
    ip_addr_t resolved;
    err_t err = dns_gethostbyname(MQTT_BROKER, &resolved, NULL, NULL);
    if (err == ERR_OK) {
        mqtt_broker_ip = resolved; // Resolução imediata
    } else if (err == ERR_INPROGRESS) {
        // Resolução assíncrona: aguarda até ~5 segundos
        for (int i=0; i<50; i++) {
            cyw43_arch_poll(); // Mantém o Wi-Fi ativo durante a espera
            err = dns_gethostbyname(MQTT_BROKER, &resolved, NULL, NULL);
            if (err == ERR_OK) { mqtt_broker_ip = resolved; break; }
            sleep_ms(100);
        }
    }
    if (err == ERR_OK) printf("DNS resolvido: %s\n", ipaddr_ntoa(&mqtt_broker_ip));
    else printf("Falha DNS (err=%d)\n", err);
    return (err == ERR_OK);
}

// ========== INICIALIZAÇÃO DO WI-FI E MQTT ==========
void wifi_init_and_connect() {
    // Inicializa o subsistema Wi-Fi (cyw43)
    if (cyw43_arch_init()) { printf("Falha init Wi-Fi\n"); return; }
    cyw43_arch_enable_sta_mode(); // Modo estação (cliente)
    printf("Conectando Wi-Fi %s...\n", WIFI_SSID);
    // Tenta conectar com timeout de 20 segundos
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha conexao Wi-Fi\n"); return;
    }
    wifi_connected = true;
    printf("Wi-Fi IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    // Resolve endereço do broker MQTT
    if (!resolve_mqtt_broker()) return;

    // Cria e conecta cliente MQTT
    mqtt_client = mqtt_client_new();
    if (mqtt_client) {
        struct mqtt_connect_client_info_t ci = { .client_id = "genius-lab", .keep_alive = 60 };
        cyw43_arch_lwip_begin(); // Protege acesso ao lwIP
        err_t e = mqtt_client_connect(mqtt_client, &mqtt_broker_ip, LWIP_IANA_PORT_MQTT,
                                      mqtt_connection_cb, NULL, &ci);
        cyw43_arch_lwip_end();
        if (e != ERR_OK) printf("Erro MQTT connect (%d)\n", e);
    }
}

// ========== PUBLICAÇÃO DE MENSAGEM MQTT ==========
void mqtt_publish_score(uint32_t score) {
    if (!wifi_connected || !mqtt_client) return; // Só publica se conectado
    sprintf(mqtt_payload, "{\"high_score\": %lu}", score); // Monta JSON simples
    cyw43_arch_lwip_begin();
    mqtt_publish(mqtt_client, MQTT_TOPIC, mqtt_payload, strlen(mqtt_payload), 0, 0, mqtt_publish_cb, NULL);
    cyw43_arch_lwip_end();
}

// ========== INICIALIZAÇÃO DO HARDWARE ==========
void init_hardware() {
    adc_init(); adc_gpio_init(JOY_X); adc_gpio_init(JOY_Y); // ADC para joystick
    // Botões com pull-up
    gpio_init(BUTTON_A); gpio_set_dir(BUTTON_A, GPIO_IN); gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B); gpio_set_dir(BUTTON_B, GPIO_IN); gpio_pull_up(BUTTON_B);
    gpio_init(JOY_BTN); gpio_set_dir(JOY_BTN, GPIO_IN); gpio_pull_up(JOY_BTN);
    // Interrupção no botão do joystick (borda de descida)
    gpio_set_irq_enabled_with_callback(JOY_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_callback);

    buzzer_init(); // Configura pinos dos buzzers

    // Inicializa I2C para OLED (400 kHz)
    i2c_init(OLED_I2C, 400*1000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C); gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA); gpio_pull_up(OLED_SCL);
    ssd1306_init(OLED_I2C);

    // Configura PIO para WS2812 (800 kHz)
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
    dma_chan = dma_claim_unused_channel(true); // Aloca canal DMA
    clear_leds();
}

// ========== CONTROLE DOS BUZZERS (VOLUME ORIGINAL) ==========
void buzzer_init() {
    // Inicializa pinos como saída digital em nível baixo
    gpio_init(BUZZER_LEFT_PIN);  gpio_set_dir(BUZZER_LEFT_PIN, GPIO_OUT);  gpio_put(BUZZER_LEFT_PIN, 0);
    gpio_init(BUZZER_RIGHT_PIN); gpio_set_dir(BUZZER_RIGHT_PIN, GPIO_OUT); gpio_put(BUZZER_RIGHT_PIN, 0);
}

// Toca um tom em um dos buzzers (alterna automaticamente a cada chamada)
void buzzer_play_tone(uint16_t freq, uint32_t ms) {
    if (freq == 0) { sleep_ms(ms); return; } // Frequência zero = silêncio
    // Seleciona qual buzzer usar (alterna entre esquerdo e direito)
    uint pin = (current_buzzer == 0) ? BUZZER_LEFT_PIN : BUZZER_RIGHT_PIN;
    current_buzzer = !current_buzzer; // Inverte para próxima chamada
    gpio_set_function(pin, GPIO_FUNC_PWM); // Habilita função PWM no pino
    uint slice = pwm_gpio_to_slice_num(pin);
    uint32_t div = 4;
    uint32_t wrap = clock_get_hz(clk_sys) / (div * freq);
    pwm_set_clkdiv(slice, div);
    pwm_set_wrap(slice, wrap);
    pwm_set_gpio_level(pin, wrap / 8);    // Duty cycle 12.5% (volume original, sem aquecimento)
    pwm_set_enabled(slice, true);
    sleep_ms(ms);
    pwm_set_enabled(slice, false);
    // Retorna pino para GPIO comum (evita ruídos)
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_OUT); gpio_put(pin, 0);
}

// Wrapper para manter compatibilidade com o restante do código
void play_tone(uint16_t freq, uint32_t ms) { buzzer_play_tone(freq, ms); }

// ========== CALLBACK DE INTERRUPÇÃO DO BOTÃO DO JOYSTICK ==========
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == JOY_BTN) {
        uint64_t now = time_us_64()/1000; // Tempo atual em ms
        // Debounce: aceita apenas a cada 200 ms
        if (now - last_irq > 200) { irq_flag = true; last_irq = now; }
    }
}

// ========== CONTROLE DOS LEDs WS2812 VIA DMA ==========
// Acende um conjunto de LEDs com uma cor, usando DMA
void set_led_color_dma(uint32_t color, const uint8_t *idx, uint8_t cnt) {
    static uint32_t px[NUM_PIXELS]; // Buffer estático (evita alocação na pilha)
    for (int i=0;i<NUM_PIXELS;i++) px[i]=0; // Zera todos
    // Atribui a cor aos índices especificados (desloca 8 bits para formato GRB)
    for (int i=0;i<cnt;i++) px[idx[i]] = color << 8u;
    // Configura e inicia transferência DMA para o FIFO do PIO
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &cfg, &pio->txf[sm], px, NUM_PIXELS, true);
}

// Apaga todos os LEDs (envia array de zeros via DMA)
void clear_leds(void) {
    static uint32_t black[NUM_PIXELS] = {0};
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &cfg, &pio->txf[sm], black, NUM_PIXELS, true);
}

// ========== REPRODUÇÃO DE MELODIAS ==========
void play_melody(const uint16_t *notes, const uint32_t *durs, size_t len) {
    for (size_t i=0; i<len; i++) {
        if (notes[i]) buzzer_play_tone(notes[i], durs[i]); // Toca nota
        else sleep_ms(durs[i]);                            // Pausa
        sleep_ms(30); // Pequena pausa entre notas (articulação)
    }
}

// ========== ATUALIZAÇÃO DO DISPLAY OLED ==========
void update_oled(const char *l1, const char *l2, const char *l3) {
    ssd1306_clear();
    ssd1306_draw_string(0,0,l1); ssd1306_draw_string(0,20,l2); ssd1306_draw_string(0,40,l3);
    if (wifi_connected) ssd1306_draw_string(100,0,"WiFi"); // Indicador de Wi-Fi no canto superior direito
    ssd1306_show();
}
void display_high_score(void) {
    char b[20]; sprintf(b, "Recorde: %d", high_score);
    ssd1306_draw_string(0,56,b); ssd1306_show(); // Exibe no rodapé
}

// ========== MECÂNICA DO JOGO ==========
// Gera um novo passo aleatório e adiciona ao final da sequência
void generate_step() {
    sequence[seq_len-1] = rand() % NUM_COLORS;
    printf("Seq len %d: ", seq_len);
    for (int i=0;i<seq_len;i++) printf("%d ", sequence[i]);
    printf("\n");
}

// Reproduz a sequência atual (luzes e sons) conforme dificuldade selecionada
void play_sequence() {
    const difficulty_config_t *c = &difficulties[current_diff]; // Configuração atual
    char b[20]; sprintf(b, "Nivel: %d (%s)", seq_len, c->name);
    update_oled("Mostrando...", b, ""); display_high_score();
    sleep_ms(500);
    for (int i=0; i<seq_len; i++) {
        uint8_t col = sequence[i];
        set_led_color_dma(COLORS[col], c->quadrants[col], c->led_count[col]);
        play_tone(TONES[col], c->tone_duration_ms);
        clear_leds(); sleep_ms(c->pause_ms);
    }
}

// Lê a entrada do jogador (joystick + botão A)
uint8_t read_input() {
    adc_select_input(0); uint16_t x = adc_read(); // Eixo X
    adc_select_input(1); uint16_t y = adc_read(); // Eixo Y
    bool btn = !gpio_get(BUTTON_A);               // Botão A pressionado?
    uint8_t sel = 255; // Valor inválido
    // Interpreta direção (limiares ajustados empiricamente)
    if (x > 3200) sel = 0; else if (x < 800) sel = 1;
    else if (y > 3200) sel = 2; else if (y < 800) sel = 3;
    if (sel < NUM_COLORS && btn) {
        sleep_ms(20); // Debounce
        if (!gpio_get(BUTTON_A)) {
            const difficulty_config_t *c = &difficulties[current_diff];
            set_led_color_dma(COLORS[sel], c->quadrants[sel], c->led_count[sel]); // Feedback visual
            while(!gpio_get(BUTTON_A)); // Aguarda soltar
            clear_leds(); return sel;
        }
    }
    sleep_ms(10); return 255;
}

// Verifica se a cor selecionada pelo jogador está correta
bool check_input(uint8_t sel) {
    bool ok = (sel == sequence[player_step]);
    printf("Esperado %d, recebido %d -> %s\n", sequence[player_step], sel, ok?"OK":"ERRO");
    return ok;
}

// Fim de jogo: exibe mensagem e toca melodia de game over
void game_over() {
    game_state = GAME_OVER;
    printf("GAME OVER\n");
    update_oled("GAME OVER", "Pressione B", "p/ reiniciar");
    play_melody(over_notes, over_durs, OVER_LEN);
    clear_leds();
}

// ========== GERENCIAMENTO DO HIGH SCORE NA FLASH ==========
void load_high_score() {
    flash_data_t *p = (flash_data_t*)FLASH_HIGH_SCORE_ADDR;
    high_score = (p->magic == FLASH_MAGIC) ? p->high_score : 0; // Valida magic number
}
void save_high_score(uint32_t new_score) {
    flash_data_t data = { FLASH_MAGIC, new_score };
    uint32_t ints = save_and_disable_interrupts(); // Protege operação na flash
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t*)&data, sizeof(data));
    restore_interrupts(ints);
    printf("High score %d salvo.\n", new_score);
}

// ========== FUNÇÃO PRINCIPAL ==========
int main() {
    stdio_init_all();
    srand(time_us_64()); // Semente aleatória baseada no tempo
    init_hardware();
    load_high_score();
    printf("\n=== GENIUS LAB ===\nHigh Score: %d\n", high_score);

    wifi_init_and_connect(); // Inicia Wi-Fi e MQTT em segundo plano
    play_melody(startup_notes, startup_durs, STARTUP_LEN); // Melodia de abertura

    char menu_buf[20];
    sprintf(menu_buf, "Dificuldade: %s", difficulties[current_diff].name);
    update_oled("Genius Lab", "A: iniciar", menu_buf);
    display_high_score();

    while (1) {
        cyw43_arch_poll(); // Mantém o Wi-Fi ativo (necessário para o driver CYW43)

        // Alterna dificuldade com botão B no menu
        if (game_state == MENU && !gpio_get(BUTTON_B)) {
            sleep_ms(50);
            if (!gpio_get(BUTTON_B)) {
                current_diff = (current_diff+1) % 3; // Cicla entre Fácil, Normal, Difícil
                sprintf(menu_buf, "Dificuldade: %s", difficulties[current_diff].name);
                update_oled("Genius Lab", "A: iniciar", menu_buf);
                display_high_score();
                while(!gpio_get(BUTTON_B)); // Aguarda soltar
            }
        }

        // Trata interrupção do botão do joystick (iniciar/pausar jogo)
        if (irq_flag) {
            irq_flag = false;
            if (game_state == MENU || game_state == GAME_OVER) {
                game_state = SEQUENCE_SHOW;
                seq_len = 1; player_step = 0;
                generate_step();
                printf("Novo jogo (%s)\n", difficulties[current_diff].name);
                play_melody(start_notes, start_durs, START_LEN);
            } else {
                game_state = MENU;
                update_oled("Jogo Pausado", "A: iniciar", menu_buf);
            }
        }

        // Máquina de estados principal
        switch (game_state) {
            case MENU: sleep_ms(10); break;
            case SEQUENCE_SHOW:
                play_sequence();
                game_state = PLAYER_INPUT; player_step = 0;
                update_oled("Sua vez!", "Repita a", "sequencia");
                break;
            case PLAYER_INPUT: {
                uint8_t c = read_input();
                if (c < NUM_COLORS) {
                    if (check_input(c)) {
                        player_step++;
                        if (player_step == seq_len) {
                            // Completou nível
                            if (seq_len > high_score) {
                                high_score = seq_len;
                                save_high_score(high_score); // Salva na flash
                                mqtt_publish_score(high_score); // Publica via MQTT
                            }
                            seq_len++; player_step = 0;
                            generate_step();
                            char b[20]; sprintf(b, "Nivel: %d", seq_len);
                            update_oled("Parabens!", b, "Proximo...");
                            play_tone(TONE_SUCCESS, 200);
                            sleep_ms(1000);
                            game_state = SEQUENCE_SHOW;
                        } else {
                            char b[20]; sprintf(b, "Passo %d/%d", player_step, seq_len);
                            update_oled("Continue...", b, "");
                        }
                    } else game_over();
                }
                break;
            }
            case GAME_OVER:
                if (!gpio_get(BUTTON_B)) {
                    sleep_ms(50);
                    if (!gpio_get(BUTTON_B)) {
                        game_state = MENU;
                        update_oled("Genius Lab", "A: iniciar", menu_buf);
                        display_high_score();
                        while(!gpio_get(BUTTON_B));
                    }
                }
                sleep_ms(10); break;
        }
    }
}