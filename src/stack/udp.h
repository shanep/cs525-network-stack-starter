/*
 * UDP on top of IP. The fixed interface is every function below. The udp_t
 * struct is yours.
 */
#ifndef UDP_H
#define UDP_H

#include "ip.h"

#define UDP_HDR_LEN 8U
#define UDP_MAX_BINDS 8
/** The most a datagram can carry without IP fragmenting it. */
#define UDP_MAX_PAYLOAD (IP_MTU - IP_HDR_LEN - UDP_HDR_LEN)

/**
 * @brief Called with each datagram for a bound port.
 * @param ctx The context you gave udp_bind().
 * @param src Source address, host order.
 * @param sport Source port.
 * @param data The payload, which is only valid until the call returns.
 * @param len The payload's length.
 */
typedef void (*udp_recv_fn)(void *ctx, uint32_t src, uint16_t sport, const uint8_t *data, size_t len);

/** @brief All of UDP's state. */
typedef struct {
  ip_t *ip; // and everything else you need
} udp_t;

/**
 * @brief Start UDP and register it with IP.
 * @return false if IP has no room for another handler.
 */
bool udp_init(udp_t *udp, ip_t *ip);

/**
 * @brief Say who gets a port's datagrams.
 * @return false if the port is 0 or taken, or there is no room.
 */
bool udp_bind(udp_t *udp, uint16_t port, udp_recv_fn fn, void *ctx);

/** @brief Give a port up. Doing this to a port that is not bound does nothing. */
void udp_unbind(udp_t *udp, uint16_t port);

/**
 * @brief Send one datagram.
 * @return false if it is too big or IP could not send it.
 */
bool udp_send(udp_t *udp, uint16_t sport, uint32_t dst, uint16_t dport, const void *data, size_t len);

#endif // UDP_H
