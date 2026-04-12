# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# ====================================================================================
# Generated Cmake Pico project file

# Define a versão mínima do CMake exigida para o projeto
cmake_minimum_required(VERSION 3.13)

# Define o padrão da linguagem C (C11) para garantir compatibilidade
set(CMAKE_C_STANDARD 11)

# Define o padrão da linguagem C++ (C++17) para garantir compatibilidade
set(CMAKE_CXX_STANDARD 17)

# Gera um arquivo compile_commands.json, útil para ferramentas de análise de código e IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicializa o pico_sdk a partir do local de instalação
# (pode ser obtido do ambiente, cache do CMake, etc.)

# ========== NÃO EDITAR AS LINHAS ABAIXO - necessário para extensão do VS Code ==========
if(WIN32)
    # No Windows, captura o diretório do perfil do usuário através da variável USERPROFILE
    set(USERHOME $ENV{USERPROFILE})
else()
    # Em sistemas Unix-like, captura o diretório home através da variável HOME
    set(USERHOME $ENV{HOME})
endif()

# Define a versão do SDK da Raspberry Pi Pico que será utilizada
set(sdkVersion 2.2.0)

# Define a versão da toolchain (compilador e ferramentas) a ser usada
set(toolchainVersion 14_2_Rel1)

# Define a versão da ferramenta picotool (utilitário para comunicação com a placa)
set(picotoolVersion 2.2.0-a4)

# Define o caminho para o arquivo de configuração específico do VS Code para Pico
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

# Verifica se o arquivo de configuração do VS Code existe e, se sim, inclui-o no projeto
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Define a placa alvo como Raspberry Pi Pico W (com Wi-Fi)
# A opção CACHE permite que o valor seja armazenado e alterado pela interface gráfica do CMake
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Inclui o arquivo de importação do SDK da Raspberry Pi Pico
# (este arquivo deve estar presente no diretório do projeto ou no SDK)
# Deve ser chamado antes de definir o projeto
include(pico_sdk_import.cmake)

# Declara o projeto CMake com nome "Genius_Lab" e as linguagens C, C++ e Assembly
project(Genius_Lab C CXX ASM)

# Inicializa o SDK da Raspberry Pi Pico, configurando as variáveis e caminhos necessários
pico_sdk_init()

# Cria um executável com o nome "Genius_Lab" a partir dos arquivos fonte especificados
add_executable(Genius_Lab Genius_Lab.c ssd1306.c )

# Gera um arquivo de cabeçalho C a partir de um programa PIO (Programmable I/O)
# O primeiro argumento é o nome do arquivo gerado (Genius_Lab.pio.h), o segundo é o caminho do arquivo .pio
pico_generate_pio_header(Genius_Lab ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Define o nome do programa que aparecerá no menu USB ou em identificações
pico_set_program_name(Genius_Lab "Genius_Lab")

# Define a versão do programa (exibida no dispositivo USB)
pico_set_program_version(Genius_Lab "0.1")

# Habilita/desabilita a saída padrão via UART (0 = desabilitado)
pico_enable_stdio_uart(Genius_Lab 0)

# Habilita/desabilita a saída padrão via USB CDC (1 = habilitado)
pico_enable_stdio_usb(Genius_Lab 1)

# Vincula a biblioteca padrão do SDK (pico_stdlib) ao executável
target_link_libraries(Genius_Lab
        pico_stdlib)

# Adiciona o diretório atual à lista de diretórios de inclusão do compilador
target_include_directories(Genius_Lab PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Vincula bibliotecas de hardware adicionais do SDK para acesso a periféricos específicos
target_link_libraries(Genius_Lab 
    hardware_i2c        # Para comunicação I2C (ex: display OLED)
    hardware_adc        # Para leitura de conversor analógico-digital (ADC)
    hardware_pwm        # Para geração de sinais PWM (ex: controle de LEDs, motores)
    hardware_gpio       # Para manipulação direta de pinos GPIO
    hardware_timer      # Para uso de temporizadores de hardware
    hardware_pio        # Para programação de blocos PIO (ex: WS2812)
    hardware_clocks     # Para configuração de clocks do sistema
    hardware_dma        # Para transferências de dados via DMA (acesso direto à memória, liberando CPU)
    hardware_flash      # Para leitura e escrita na memória flash interna (ex: salvar recordes)
    hardware_sync       # Para controle de interrupções e operações atômicas (ex: proteger escrita na flash)
    pico_cyw43_arch_lwip_poll   # Suporte ao chip Wi‑Fi CYW43439 e polling do lwIP (necessário para manter a conexão)
    pico_lwip_mqtt      # Cliente MQTT sobre lwIP (publicação de recordes online)
)

# Gera arquivos de saída adicionais como .uf2, .elf, .bin, etc., para gravação na placa
pico_add_extra_outputs(Genius_Lab)