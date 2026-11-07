#include "port.h"

uint32_t net_checksum_add(uint32_t sum, const void *data, size_t len) {
  (void)sum; (void)data; (void)len; return 0;
}

uint16_t net_checksum_finish(uint32_t sum) {
  (void)sum; return 0;
}

uint32_t net_checksum_pseudo(uint32_t sum, uint32_t src, uint32_t dst, uint8_t proto, uint16_t len) {
  (void)sum; (void)src; (void)dst; (void)proto; (void)len; return 0;
}
