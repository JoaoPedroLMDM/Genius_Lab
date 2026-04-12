#ifndef _LWIPOPTS_EXAMPLE_COMMONH_H
#define _LWIPOPTS_EXAMPLE_COMMONH_H

// ============================================================================
// ARQUIVO DE CONFIGURAÇÃO DO lwIP (Lightweight IP) PARA O RASPBERRY PI PICO W
// ============================================================================
// Este arquivo define as opções de compilação da pilha TCP/IP lwIP, otimizadas
// para o uso com o chip Wi‑Fi CYW43439 no modo estação (polling ou callback).
// Documentação oficial: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html
// ============================================================================

// ========== CONFIGURAÇÕES GERAIS DO SISTEMA ==========
// Define se o lwIP deve operar com um sistema operacional (NO_SYS = 1 → sem SO).
// Valor padrão para a maioria dos exemplos do Pico W.
#ifndef NO_SYS
#define NO_SYS                      1   // Sem camada de sistema operacional (bare metal)
#endif

// Habilita/desabilita a API de sockets BSD (0 = desabilitada).
// Como usamos MQTT diretamente, sockets não são necessários, economizando memória.
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0   // Desabilita API de sockets
#endif

// ========== GERENCIAMENTO DE MEMÓRIA ==========
// Quando o driver Wi‑FI opera em modo POLL (checagem constante pela CPU),
// o malloc da libc pode ser usado, o que é mais eficiente em alguns cenários.
// No modo de interrupção (callback), MEM_LIBC_MALLOC deve ser 0.
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1   // Usa malloc() da biblioteca C padrão
#else
#define MEM_LIBC_MALLOC             0   // Usa gerenciador de memória interno do lwIP
#endif

// Alinhamento da memória (4 bytes para o ARM Cortex-M0+ do RP2040).
#define MEM_ALIGNMENT               4

// Tamanho total do heap do lwIP em bytes. Aumentar se houver muitas conexões TCP.
#ifndef MEM_SIZE
#define MEM_SIZE                    4000  // 4 KB de heap para o lwIP
#endif

// Número máximo de segmentos TCP que podem ser alocados simultaneamente.
#define MEMP_NUM_TCP_SEG            32

// Número de entradas na fila de cache ARP.
#define MEMP_NUM_ARP_QUEUE          10

// Número de buffers de pacote (pbuf) disponíveis. Cada pbuf pode armazenar um pacote de rede.
#define PBUF_POOL_SIZE              24

// ========== PROTOCOLOS HABILITADOS ==========
#define LWIP_ARP                    1   // Habilita resolução ARP (necessário para IPv4 sobre Ethernet/Wi‑Fi)
#define LWIP_ETHERNET               1   // Habilita camada Ethernet (usada pelo Wi‑Fi)
#define LWIP_ICMP                   1   // Habilita ICMP (ex.: ping)
#define LWIP_RAW                    1   // Habilita API RAW (necessária para algumas aplicações de baixo nível)

// ========== PARÂMETROS DO TCP ==========
// Tamanho da janela de recepção TCP.
#define TCP_WND                     (8 * TCP_MSS)

// Maximum Segment Size (tamanho máximo de um segmento TCP).
#define TCP_MSS                     1460  // Tamanho típico para Ethernet/Wi‑Fi (MTU 1500 - cabeçalhos)

// Tamanho do buffer de envio TCP.
#define TCP_SND_BUF                 (8 * TCP_MSS)

// Comprimento da fila de envio TCP (número de pacotes que podem ficar aguardando).
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// ========== CALLBACKS DE STATUS DA INTERFACE DE REDE ==========
#define LWIP_NETIF_STATUS_CALLBACK  1   // Permite receber notificações quando a interface sobe/desce
#define LWIP_NETIF_LINK_CALLBACK    1   // Permite receber notificações sobre mudanças no link (ex.: Wi‑Fi conectado)
#define LWIP_NETIF_HOSTNAME         1   // Habilita definição de hostname via DHCP

// ========== OUTRAS CONFIGURAÇÕES ==========
#define LWIP_NETCONN                0   // Desabilita API Netconn (não usada)
#define MEM_STATS                   0   // Desabilita estatísticas de memória (economiza código)
#define SYS_STATS                   0   // Desabilita estatísticas de sistema
#define MEMP_STATS                  0   // Desabilita estatísticas de pools de memória
#define LINK_STATS                  0   // Desabilita estatísticas de link

// Algoritmo de checksum (3 = checksum por hardware quando possível, senão software).
#define LWIP_CHKSUM_ALGORITHM       3

// ========== PROTOCOLOS DE REDE ADICIONAIS ==========
#define LWIP_DHCP                   1   // Habilita cliente DHCP (obter IP automaticamente)
#define LWIP_IPV4                   1   // Habilita suporte a IPv4
#define LWIP_TCP                    1   // Habilita protocolo TCP
#define LWIP_UDP                    1   // Habilita protocolo UDP
#define LWIP_DNS                    1   // Habilita resolução de nomes DNS (necessário para MQTT com hostname)

// ========== CONFIGURAÇÕES ESPECÍFICAS PARA O PICO W ==========
#define LWIP_TCP_KEEPALIVE          1   // Habilita keep‑alive do TCP
#define LWIP_NETIF_TX_SINGLE_PBUF   1   // Otimização: transmite cada pacote em um único pbuf
#define DHCP_DOES_ARP_CHECK         0   // Desabilita verificação ARP durante DHCP (evita atrasos)
#define LWIP_DHCP_DOES_ACD_CHECK    0   // Desabilita Address Conflict Detection no DHCP

// ========== CONFIGURAÇÕES DE DEBUG (APENAS EM MODO NDEBUG) ==========
// Quando NDEBUG não está definido (build de debug), habilita saída de depuração do lwIP.
#ifndef NDEBUG
#define LWIP_DEBUG                  1   // Habilita mensagens de debug do lwIP
#define LWIP_STATS                  1   // Habilita coleta de estatísticas
#define LWIP_STATS_DISPLAY          1   // Habilita exibição das estatísticas
#endif

// ========== DESABILITA TODAS AS MENSAGENS DE DEBUG INDIVIDUALMENTE ==========
// Mesmo com LWIP_DEBUG habilitado, cada módulo pode ter seu nível de verbosidade.
// Aqui todos são configurados como LWIP_DBG_OFF (nenhuma saída).
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */