#include "udp.h"


bool udp_init(udp_t *udp, ip_t *ip) {
  (void)udp; (void)ip; return false;
}

bool udp_bind(udp_t *udp, uint16_t port, udp_recv_fn fn, void *ctx) {
  (void)udp; (void)port; (void)fn; (void)ctx; return false;
}

void udp_unbind(udp_t *udp, uint16_t port) {
  (void)udp; (void)port;
}

bool udp_send(udp_t *udp, uint16_t sport, uint32_t dst, uint16_t dport, const void *data, size_t len) {
  (void)udp; (void)sport; (void)dst; (void)dport; (void)data; (void)len; return false;
}
