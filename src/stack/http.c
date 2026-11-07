#include "http.h"


uint8_t http_pattern(uint32_t offset) {
  (void)offset; return 0;
}

bool http_init(http_t *http, tcp_t *tcp, uint16_t port) {
  (void)http; (void)tcp; (void)port; return false;
}

void http_poll(http_t *http) {
  (void)http;
}

uint32_t http_served(const http_t *http) {
  (void)http; return 0;
}
