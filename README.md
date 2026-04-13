# 🎮 Genius Lab – Jogo da Memória para BitDogLab

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![EmbarcaTech](https://img.shields.io/badge/EmbarcaTech-Projeto%20Final-blue)](https://embarcatech.softex.br)
[![Raspberry Pi Pico](https://img.shields.io/badge/Platform-RP2040-green)](https://www.raspberrypi.com/products/rp2040/)
[![Wi-Fi](https://img.shields.io/badge/Wi--Fi-CYW43439-orange)]()
[![MQTT](https://img.shields.io/badge/MQTT-broker.hivemq.com-purple)]()

**Projeto Final do Programa EmbarcaTech – Expansão**  
Desenvolvido para a placa **BitDogLab (RP2040)**, o Genius Lab é um jogo da memória eletrônico inspirado no clássico _Genius_. O sistema utiliza os periféricos integrados da placa e atende a todos os requisitos obrigatórios do edital, incluindo **DMA (bônus)** e **conectividade Wi‑Fi com publicação MQTT**.

<p align="center">
  <img src="assets/demo.gif" alt="Demonstração do Genius Lab" width="500"/>
</p>

---

## ✨ Funcionalidades

- 🧠 Jogo da memória com 4 cores (verde, azul, amarelo, vermelho)
- 📟 Display OLED 128×64 (I2C) exibindo nível, status, dificuldade e recorde
- 💡 Matriz de LEDs WS2812B (5×5) como saída visual
- 🔊 Dois buzzers PWM com melodias (abertura, início de jogo e game over)
- 💾 High Score salvo em memória Flash (persistente)
- ⚡ **DMA** para controle da matriz de LEDs
- 🎛️ Interrupção (IRQ) no botão do joystick para iniciar/pausar
- 📡 Comunicação UART via USB para depuração (`printf`)
- 🌐 Conectividade Wi‑Fi e publicação de recordes via **MQTT**

---

## ✅ Requisitos Atendidos

| Requisito do Edital | Status |
|---------------------|--------|
| Placa BitDogLab (RP2040) | ✅ |
| Programação em C/C++ (Pico SDK) | ✅ |
| Dois periféricos do kit (OLED + Matriz WS2812B) | ✅ |
| Comunicação UART | ✅ |
| Uso de Interrupção (IRQ) | ✅ |
| Uso de DMA | ✅ |
| Código documentado e organizado | ✅ |
| Repositório público com README e licença | ✅ |

---

## 🎮 Como Jogar

1. Ligue a BitDogLab e aguarde a conexão Wi‑Fi (indicada no OLED).
2. No menu, pressione **Botão B** para alternar entre os níveis: **Fácil**, **Normal** e **Difícil**.
3. Pressione o **botão do joystick** para iniciar a partida.
4. Observe a sequência de cores exibida na matriz e os sons correspondentes.
5. Repita a sequência usando as direções do joystick e confirme com o **Botão A**.
   - ⬅️ Esquerda → Amarelo  
   - ➡️ Direita → Verde  
   - ⬆️ Cima → Vermelho  
   - ⬇️ Baixo → Azul  
6. A cada rodada completa, uma nova cor é adicionada à sequência.
7. Se errar, o jogo exibe **GAME OVER** e toca uma melodia de derrota. Pressione **Botão B** para voltar ao menu.
8. O recorde é salvo automaticamente na flash e, se for superado, publicado na nuvem via MQTT.

---

## 🔌 Pinagem Utilizada (BitDogLab)

| Componente | Pino / Interface | Função |
|------------|------------------|--------|
| Matriz WS2812B | GPIO7 (PIO + DMA) | Saída visual |
| Display OLED | I2C (SDA: GPIO14, SCL: GPIO15) | Interface de status |
| Botão A | GPIO5 | Confirmar cor |
| Botão B | GPIO6 | Alternar dificuldade / Reiniciar |
| Botão do Joystick | GPIO22 (IRQ) | Iniciar / Pausar |
| Joystick (X/Y) | ADC0 (GPIO26) / ADC1 (GPIO27) | Seleção de direção |
| Buzzer Esquerdo | GPIO21 (PWM) | Feedback sonoro |
| Buzzer Direito | GPIO10 (PWM) | Feedback sonoro (alternado) |
| Wi‑Fi | CYW43439 (SPI interna) | Conexão de rede e MQTT |

---

## 🔧 Compilação e Execução

### Pré‑requisitos
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (versão 2.2.0 ou superior)
- CMake ≥ 3.13
- Compilador ARM (`arm-none-eabi-gcc`)
- Conexão com a Internet para baixar os submódulos do SDK

### Passos
```bash
git clone https://github.com/JoaoPedroLMDM/genius-lab-bitdoglab.git
cd genius-lab-bitdoglab
mkdir build && cd build
cmake ..
make -j4
