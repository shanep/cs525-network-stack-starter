/*
 * IPv4 and ICMP. This is a host, so it never forwards. Everything above it
 * registers a handler for its protocol number, and everything below it is one
 * function pointer, so you can test all of this before Ethernet exists. The
 * fixed interface is every function below. The ip_t struct is yours.
 */
#ifndef IP_H
#define IP_H

#include "port.h"

#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP 17
#define IP_HDR_LEN 20U
/** The largest packet we send, which is Ethernet's MTU. */
#define IP_MTU 1500U
#define IP_TTL 64
#define IP_MAX_ROUTES 8
#define IP_MAX_HANDLERS 4
#define IP_BROADCAST 0xFFFFFFFFU

/** ICMP destination unreachable codes that you send. */
#define ICMP_PROTO_UNREACHABLE 2
#define ICMP_PORT_UNREACHABLE 3

/**
 * @brief Where finished packets go, Ethernet in the real stack.
 * @param ctx The context you gave ip_init().
 * @param next_hop The address the link layer has to reach, host order.
 * @param packet The whole packet, which is only valid until the call returns.
 * @param len The packet's length.
 */
typedef void (*ip_link_fn)(void *ctx, uint32_t next_hop, const uint8_t *packet, size_t len);

/**
 * @brief Called with the payload of a packet for this host.
 * @param ctx The context you gave ip_set_handler().
 * @param src Source address, host order.
 * @param dst Destination address, host order.
 * @param payload The payload, which is only valid until the call returns.
 * @param len The payload's length, from the IP header and not the frame.
 */
typedef void (*ip_handler_fn)(void *ctx, uint32_t src, uint32_t dst, const uint8_t *payload, size_t len);

/** @brief What happened to the packets that came in. */
typedef struct {
  uint32_t delivered;
  uint32_t bad_header;   /**< too short, wrong version, bad lengths, bad checksum */
  uint32_t not_for_us;
  uint32_t fragments;    /**< we do not reassemble, so these are dropped */
  uint32_t no_handler;
  uint32_t no_route;     /**< packets we could not send */
} ip_stats_t;

/** @brief All of IP's state. */
typedef struct {
  uint32_t addr; ip_stats_t stats; // and everything else you need
} ip_t;

/**
 * @brief Start IP with no routes and no handlers.
 * @param ip The state to fill in.
 * @param link Where finished packets go.
 * @param ctx Handed back to link on every call.
 * @param addr This host's address, host order.
 */
void ip_init(ip_t *ip, ip_link_fn link, void *ctx, uint32_t addr);

/**
 * @brief Add a route.
 * @param prefix The network, host order.
 * @param prefix_len The prefix length, 0 to 32. A length of 0 is the default route.
 * @param gateway The next hop, or 0 if the network is directly attached.
 * @return false if the length is over 32 or the table is full.
 */
bool ip_route_add(ip_t *ip, uint32_t prefix, uint8_t prefix_len, uint32_t gateway);

/**
 * @brief Find the next hop by longest prefix match.
 * @param ip The state.
 * @param dst Where the packet is going.
 * @param next_hop Set to the gateway, or to dst if it is directly attached.
 * @return false if no route matches.
 */
bool ip_route_lookup(const ip_t *ip, uint32_t dst, uint32_t *next_hop);

/**
 * @brief Say who gets a protocol's packets.
 * @return false if there is no room.
 */
bool ip_set_handler(ip_t *ip, uint8_t proto, ip_handler_fn fn, void *ctx);

/**
 * @brief Wrap a payload in an IP header and send it.
 * @return false if it is too big to send or there is no route.
 */
bool ip_output(ip_t *ip, uint8_t proto, uint32_t dst, const void *payload, size_t len);

/**
 * @brief Hand IP one packet from the link layer. Anything malformed is dropped.
 *
 * The packet can be longer than its header says, because Ethernet pads short
 * frames, so only trust the total length in the header.
 */
void ip_input(ip_t *ip, const uint8_t *packet, size_t len);

/**
 * @brief Send an ICMP destination unreachable about the packet being handled.
 *
 * Only a handler can call this, while ip_input() is running. RFC 1122 says to
 * stay quiet about broadcasts and about ICMP itself, or two hosts can get stuck
 * sending errors about each other's errors.
 *
 * @param code Which kind of unreachable.
 */
void ip_reply_unreachable(ip_t *ip, uint8_t code);

/**
 * @brief Send an ICMP echo request.
 * @return false if it could not be sent.
 */
bool ip_ping(ip_t *ip, uint32_t dst, uint16_t id, uint16_t seq);

/** @brief How many ICMP echo replies have come back. */
uint32_t ip_ping_replies(const ip_t *ip);

/** @brief The counters. */
ip_stats_t ip_stats(const ip_t *ip);

#endif // IP_H
