/*
 * The whole stack wired together. net_init() connects the layers, and calling
 * net_poll() over and over is what makes everything run. Nothing here uses
 * interrupts or threads, so there is no locking to think about. The fixed
 * interface is every function below.
 */
#ifndef STACK_H
#define STACK_H

#include "dns.h"
#include "eth.h"
#include "ip.h"
#include "tcp.h"
#include "udp.h"

/** @brief How this host is set up. We have no DHCP client, so this is all static. */
typedef struct {
  uint8_t mac[6];
  uint32_t addr;       /**< this host */
  uint8_t prefix_len;  /**< the length of the network this host is on */
  uint32_t gateway;    /**< 0 for no default route */
  uint32_t dns;        /**< the DNS server */
  uint32_t isn;        /**< TCP's first initial sequence number */
} net_config_t;

/** @brief Every layer, over a megabyte in all, so make it static. */
typedef struct {
  netif_t netif;
  eth_t eth;
  ip_t ip;
  udp_t udp;
  dns_t dns;
  tcp_t tcp;
  uint8_t frame[1600];
} net_stack_t;

/**
 * @brief Connect all of the layers.
 * @param net The state to fill in.
 * @param netif The card, which is copied.
 * @param config How this host is set up.
 */
void net_init(net_stack_t *net, const netif_t *netif, const net_config_t *config);

/**
 * @brief Do everything that is due: tell each layer the time and take the frames
 *        that have arrived. It never blocks.
 */
void net_poll(net_stack_t *net);

#endif // STACK_H
