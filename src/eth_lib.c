#include "eth_lib.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

LOG_MODULE_REGISTER(net_ethernet_sample, LOG_LEVEL_INF);

/* Tempo máximo para aguardar por um endereço IP (em ms) */
#define DHCP_TIMEOUT K_SECONDS(30)

/* Para eventos de gerenciamento de rede */
static struct net_mgmt_event_callback mgmt_cb;

/* Interface de rede do Ethernet */
static struct net_if *iface;

/* Buffer para armazenar o endereço IP como string */
static char ip_addr[NET_IPV4_ADDR_LEN];

/* Função para converter endereço IPv4 para string */
static void net_addr_to_str(char *buf, struct in_addr *addr) {
  net_addr_ntop(AF_INET, addr, buf, NET_IPV4_ADDR_LEN);
}

/* Callback para eventos de rede */
static void net_event_handler(struct net_mgmt_event_callback *cb,
                              uint32_t mgmt_event, struct net_if *iface) {
  if (mgmt_event == NET_EVENT_IF_UP) {
    LOG_INF("Interface %p está UP", iface);
  } else if (mgmt_event == NET_EVENT_IF_DOWN) {
    LOG_INF("Interface %p está DOWN", iface);
  } else if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
    /* Obter informações do endereço IPV4 */
    struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
    struct net_if_addr *if_addr = NULL;

    for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
      if (ipv4->unicast[i].ipv4.is_used) {
        if_addr = &ipv4->unicast[i].ipv4;
        net_addr_to_str(ip_addr, &ipv4->unicast[i].ipv4.address.in_addr);
        LOG_INF("IP recebido: %s", ip_addr);
        break;
      }
    }
  } else if (mgmt_event == NET_EVENT_ETHERNET_CARRIER_ON) {
    LOG_INF("Ethernet: Cabo conectado");
  } else if (mgmt_event == NET_EVENT_ETHERNET_CARRIER_OFF) {
    LOG_INF("Ethernet: Cabo desconectado");
  }
}

int eth_init(void) {
  LOG_INF("Iniciando aplicação de exemplo Ethernet STM32H753");

  /* Obtém a interface de rede (assumindo que só temos uma) */
  iface = net_if_get_default();

  if (!iface) {
    LOG_ERR("Não foi possível obter interface de rede padrão");
    return -1;
  }

  /* Registra callbacks para eventos de rede */
  net_mgmt_init_event_callback(
      &mgmt_cb, net_event_handler,
      (NET_EVENT_IF_UP | NET_EVENT_IF_DOWN | NET_EVENT_IPV4_ADDR_ADD |
       NET_EVENT_ETHERNET_CARRIER_ON | NET_EVENT_ETHERNET_CARRIER_OFF));

  net_mgmt_add_event_callback(&mgmt_cb);

  /* Inicia o DHCP para obter o endereço IP */
  LOG_INF("Iniciando DHCP para interface %p", iface);
  net_dhcpv4_start(iface);

  return 0;
}
