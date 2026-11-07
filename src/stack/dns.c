#include "dns.h"


size_t dns_build_query(uint8_t *buf, size_t cap, uint16_t id, const char *name) {
  (void)buf; (void)cap; (void)id; (void)name; return 0;
}

bool dns_read_name(const uint8_t *msg, size_t len, size_t off, char *out, size_t cap, size_t *next) {
  (void)msg; (void)len; (void)off; (void)out; (void)cap; (void)next; return false;
}

dns_status_t dns_parse_response(const uint8_t *msg, size_t len, uint16_t id, const char *name, uint32_t *addr) {
  (void)msg; (void)len; (void)id; (void)name; (void)addr; return DNS_FAILED;
}

bool dns_init(dns_t *dns, udp_t *udp, uint32_t server, uint16_t seed) {
  (void)dns; (void)udp; (void)server; (void)seed; return false;
}

bool dns_resolve(dns_t *dns, const char *name) {
  (void)dns; (void)name; return false;
}

void dns_tick(dns_t *dns, uint32_t now_ms) {
  (void)dns; (void)now_ms;
}

dns_status_t dns_result(const dns_t *dns, uint32_t *addr) {
  (void)dns; (void)addr; return DNS_FAILED;
}
