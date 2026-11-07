#include "tcp.h"


void tcp_init(tcp_t *tcp, tcp_output_fn output_fn, void *ctx, uint32_t addr, uint32_t isn) {
  (void)tcp; (void)output_fn; (void)ctx; (void)addr; (void)isn;
}

void tcp_input(tcp_t *tcp, uint32_t src, uint32_t dst, const uint8_t *segment, size_t len) {
  (void)tcp; (void)src; (void)dst; (void)segment; (void)len;
}

void tcp_tick(tcp_t *tcp, uint32_t now_ms) {
  (void)tcp; (void)now_ms;
}

int tcp_listen(tcp_t *tcp, uint16_t port) {
  (void)tcp; (void)port; return -1;
}

int tcp_accept(tcp_t *tcp, int listener) {
  (void)tcp; (void)listener; return -1;
}

int tcp_connect(tcp_t *tcp, uint32_t dst, uint16_t port) {
  (void)tcp; (void)dst; (void)port; return -1;
}

int tcp_write(tcp_t *tcp, int conn, const void *data, size_t len) {
  (void)tcp; (void)conn; (void)data; (void)len; return -1;
}

int tcp_read(tcp_t *tcp, int conn, void *data, size_t len) {
  (void)tcp; (void)conn; (void)data; (void)len; return -1;
}

void tcp_close(tcp_t *tcp, int conn) {
  (void)tcp; (void)conn;
}

bool tcp_get_info(const tcp_t *tcp, int conn, tcp_info_t *info) {
  (void)tcp; (void)conn; (void)info; return false;
}
