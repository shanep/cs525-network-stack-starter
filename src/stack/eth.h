/*
 * Ethernet and ARP, the bottom of the stack, which sits on a netif_t. The fixed
 * interface is every function below. The eth_t struct is yours.
 */
#ifndef ETH_H
#define ETH_H

#include "netif.h"

#define ETH_HDR_LEN 14U
/** The biggest payload, and the most IP will ever hand us. */
#define ETH_MTU 1500U
/** Shorter frames get padded, not counting the CRC the card adds. */
#define ETH_MIN_FRAME 60U
#define ETH_MAX_FRAME (ETH_HDR_LEN + ETH_MTU)
#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

#define ARP_CACHE_SIZE 16
/** How long a learned address is good for. */
#define ARP_TTL_MS 60000U
#define ARP_RETRY_MS 1000U
/** Give up on an address after this many requests. */
#define ARP_MAX_TRIES 5U

/**
 * @brief Called with the payload of each IP frame for this host.
 * @param ctx The context you gave eth_init().
 * @param packet The packet, which is only valid until the call returns.
 * @param len The payload's length, which can include Ethernet's padding.
 */
typedef void (*eth_ip_fn)(void *ctx, const uint8_t *packet, size_t len);

/** @brief All of Ethernet's and ARP's state. */
typedef struct {
  uint8_t mac[6]; // and everything else you need
} eth_t;

/**
 * @brief Start Ethernet with an empty ARP cache.
 * @param eth The state to fill in.
 * @param netif The card. It has to outlive eth_t.
 * @param mac This host's hardware address.
 * @param addr This host's IP address, host order, for answering ARP.
 * @param up Who gets the IP packets.
 * @param ctx Handed back to up on every call.
 */
void eth_init(eth_t *eth, const netif_t *netif, const uint8_t mac[6], uint32_t addr, eth_ip_fn up, void *ctx);

/** @brief Hand Ethernet one frame from the card. Anything malformed or not for us is dropped. */
void eth_input(eth_t *eth, const uint8_t *frame, size_t len);

/**
 * @brief Send an IP packet to a next hop, using ARP to find it.
 *
 * If the address is not known yet, this sends an ARP request and holds on to
 * this one packet, replacing any packet already held for that address. The
 * layers above have to survive that, and TCP does by retransmitting.
 *
 * @param eth The state.
 * @param next_hop The address on this link, host order.
 * @param packet The packet.
 * @param len The packet's length, at most ETH_MTU.
 */
void eth_send_ip(eth_t *eth, uint32_t next_hop, const uint8_t *packet, size_t len);

/** @brief Tell Ethernet what time it is, which retries ARP and expires the cache. */
void eth_tick(eth_t *eth, uint32_t now_ms);

/**
 * @brief Look an address up in the ARP cache.
 * @param eth The state.
 * @param ip The address, host order.
 * @param mac Set to the hardware address if it is known.
 * @return false if it is not known.
 */
bool arp_lookup(const eth_t *eth, uint32_t ip, uint8_t mac[6]);

#endif // ETH_H
