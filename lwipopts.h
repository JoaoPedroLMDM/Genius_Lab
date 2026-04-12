// Guarda de inclusão múltipla: impede que o arquivo seja processado mais de uma vez
#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// ========== CONFIGURAÇÃO PARA SUPORTE A TLS (MQTT COM CERTIFICADO) ==========
// Quando a macro MQTT_CERT_INC está definida (indica que um certificado TLS será usado),
// o tamanho da memória heap do lwIP é aumentado para 8 KB, pois a criptografia exige mais RAM.
#ifdef MQTT_CERT_INC
#define MEM_SIZE 8000   // Aumenta heap do lwIP de ~4 KB para 8 KB (necessário para TLS)
#endif

// ========== INCLUSÃO DAS CONFIGURAÇÕES COMUNS ==========
// A maioria das opções do lwIP está definida no arquivo "lwipopts_examples_common.h".
// Isso evita repetição de código e mantém as configurações base padronizadas.
#include "lwipopts_examples_common.h"

// ========== AUMENTO DO NÚMERO DE TIMERS DO SISTEMA ==========
// O lwIP usa timers internos para tarefas como retransmissão TCP e renovação DHCP.
// Aqui adicionamos +1 timer para uso próprio (ex.: timeouts do MQTT).
#define MEMP_NUM_SYS_TIMEOUT        (LWIP_NUM_SYS_TIMEOUT_INTERNAL+1)

// ========== CONFIGURAÇÕES ESPECÍFICAS PARA TLS (APENAS SE MQTT_CERT_INC ESTIVER DEFINIDA) ==========
#ifdef MQTT_CERT_INC
// Habilita a camada de abstração para protocolos alternativos (ALTCP), necessária para TLS.
#define LWIP_ALTCP               1

// Habilita o módulo TLS dentro da camada ALTCP.
#define LWIP_ALTCP_TLS           1

// Define que a implementação do TLS será fornecida pela biblioteca mbedTLS.
#define LWIP_ALTCP_TLS_MBEDTLS   1

// Em builds de debug (NDEBUG não definido), habilita mensagens de depuração do mbedTLS.
#ifndef NDEBUG
#define ALTCP_MBEDTLS_DEBUG  LWIP_DBG_ON
#endif

// ===== AJUSTE CRÍTICO PARA TLS =====
// A janela de recepção TCP (TCP_WND) precisa ser grande o suficiente para acomodar
// um registro TLS completo (geralmente 16 KB). Caso contrário, o lwIP emitirá um aviso
// e a conexão poderá travar na recepção de dados criptografados.
// Redefinimos a macro TCP_WND para 16384 bytes (16 KB).
#undef TCP_WND
#define TCP_WND  16384

#endif // MQTT_CERT_INC

// ========== CONFIGURAÇÃO DO MQTT ==========
// Define o número máximo de mensagens MQTT "em voo" (publicadas e ainda não confirmadas).
// O valor padrão do lwIP é 4; aqui aumentamos para 5 para permitir maior throughput.
#define MQTT_REQ_MAX_IN_FLIGHT 5

#endif // _LWIPOPTS_H