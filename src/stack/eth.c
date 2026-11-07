#include "eth.h"


void eth_init(eth_t *eth, const netif_t *netif, const uint8_t mac[6], uint32_t addr, eth_ip_fn up, void *ctx) {
  (void)eth; (void)netif; (void)mac; (void)addr; (void)up; (void)ctx;
}

void eth_input(eth_t *eth, const uint8_t *frame, size_t len) {
  (void)eth; (void)frame; (void)len;
}

void eth_send_ip(eth_t *eth, uint32_t next_hop, const uint8_t *packet, size_t len) {
  (void)eth; (void)next_hop; (void)packet; (void)len;
}

void eth_tick(eth_t *eth, uint32_t now_ms) {
  (void)eth; (void)now_ms;
}

bool arp_lookup(const eth_t *eth, uint32_t ip, uint8_t mac[6]) {
  (void)eth; (void)ip; (void)mac; return false;
}
