#include "ip.h"


void ip_init(ip_t *ip, ip_link_fn link, void *ctx, uint32_t addr) {
  (void)ip; (void)link; (void)ctx; (void)addr;
}

bool ip_route_add(ip_t *ip, uint32_t prefix, uint8_t prefix_len, uint32_t gateway) {
  (void)ip; (void)prefix; (void)prefix_len; (void)gateway; return false;
}

bool ip_route_lookup(const ip_t *ip, uint32_t dst, uint32_t *next_hop) {
  (void)ip; (void)dst; (void)next_hop; return false;
}

bool ip_set_handler(ip_t *ip, uint8_t proto, ip_handler_fn fn, void *ctx) {
  (void)ip; (void)proto; (void)fn; (void)ctx; return false;
}

bool ip_output(ip_t *ip, uint8_t proto, uint32_t dst, const void *payload, size_t len) {
  (void)ip; (void)proto; (void)dst; (void)payload; (void)len; return false;
}

void ip_input(ip_t *ip, const uint8_t *packet, size_t len) {
  (void)ip; (void)packet; (void)len;
}

void ip_reply_unreachable(ip_t *ip, uint8_t code) {
  (void)ip; (void)code;
}

bool ip_ping(ip_t *ip, uint32_t dst, uint16_t id, uint16_t seq) {
  (void)ip; (void)dst; (void)id; (void)seq; return false;
}

uint32_t ip_ping_replies(const ip_t *ip) {
  (void)ip; return 0;
}

ip_stats_t ip_stats(const ip_t *ip) {
  ip_stats_t none = {0}; (void)ip; return none;
}
