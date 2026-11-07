/*
 * Run the whole stack over the simulator, with no kernel at all. Two hosts move a
 * file through a link as bad as you want, and the congestion window goes to stdout
 * as CSV, which is the picture of Reno you can plot.
 *
 *   ./build/release/myapp -l 0.01 -r 100 -d 20 -q 40 -n 2000000 > cwnd.csv
 *
 * Do NOT modify this file!
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim.h"
#include "stack.h"

#define ADDR_A IP4(10, 0, 0, 1)
#define ADDR_B IP4(10, 0, 0, 2)
#define PORT 5000
#define LIMIT_MS (30U * 60U * 1000U)

static sim_link_t wire;
static sim_port_t port_a, port_b;
static net_stack_t a, b;
static uint8_t chunk[8192];

static uint8_t byte_at(size_t i) { return (uint8_t)((i * 31U) ^ (i >> 8) ^ (i >> 16)); }

static void usage(const char *name) {
  fprintf(stderr,
          "usage: %s [-n bytes] [-d delay_ms] [-r bytes_per_ms] [-q queue_packets]\n"
          "       [-l loss] [-u duplicate] [-o reorder] [-c corrupt] [-s seed]\n",
          name);
}

int main(int argc, char **argv) {
  size_t total = 1000000;
  sim_config_t config = {.delay_ms = 20, .bytes_per_ms = 200, .queue_packets = 50, .reorder_ms = 15, .seed = 1};
  int opt;
  while ((opt = getopt(argc, argv, "n:d:r:q:l:u:o:c:s:h")) != -1) {
    switch (opt) {
      case 'n': total = strtoul(optarg, NULL, 10); break;
      case 'd': config.delay_ms = (uint32_t)strtoul(optarg, NULL, 10); break;
      case 'r': config.bytes_per_ms = (uint32_t)strtoul(optarg, NULL, 10); break;
      case 'q': config.queue_packets = (uint32_t)strtoul(optarg, NULL, 10); break;
      case 'l': config.loss = strtod(optarg, NULL); break;
      case 'u': config.duplicate = strtod(optarg, NULL); break;
      case 'o': config.reorder = strtod(optarg, NULL); break;
      case 'c': config.corrupt = strtod(optarg, NULL); break;
      case 's': config.seed = (uint32_t)strtoul(optarg, NULL, 10); break;
      default: usage(argv[0]); return opt == 'h' ? 0 : 2;
    }
  }

  sim_init(&wire, &config);
  netif_t netif_a = sim_netif(&port_a, &wire, 0);
  netif_t netif_b = sim_netif(&port_b, &wire, 1);
  net_config_t config_a = {.mac = {2, 0, 0, 0, 0, 1}, .addr = ADDR_A, .prefix_len = 24, .isn = 1000};
  net_config_t config_b = {.mac = {2, 0, 0, 0, 0, 2}, .addr = ADDR_B, .prefix_len = 24, .isn = 900000};
  net_init(&a, &netif_a, &config_a);
  net_init(&b, &netif_b, &config_b);

  int listener = tcp_listen(&b.tcp, PORT);
  int client = tcp_connect(&a.tcp, ADDR_B, PORT);
  int server = -1;
  size_t sent = 0, got = 0;
  bool closed = false, same = true, eof = false;

  printf("ms,state,cwnd,ssthresh,flight,rto_ms,retransmits,fast_retransmits,timeouts\n");
  tcp_info_t info = {0};
  uint32_t last_cwnd = 0;
  while (!eof && sim_now(&wire) < LIMIT_MS) {
    sim_advance(&wire, 1);
    net_poll(&a);
    net_poll(&b);

    if (!closed && tcp_get_info(&a.tcp, client, &info)) {
      if (info.cwnd != last_cwnd) {
        last_cwnd = info.cwnd;
        printf("%u,%d,%u,%u,%u,%u,%u,%u,%u\n", sim_now(&wire), info.state, info.cwnd, info.ssthresh, info.flight,
               info.rto_ms, info.retransmits, info.fast_retransmits, info.timeouts);
      }
      if (info.state == TCP_ESTABLISHED) {
        while (sent < total) {
          size_t n = total - sent < sizeof(chunk) ? total - sent : sizeof(chunk);
          for (size_t i = 0; i < n; i++) chunk[i] = byte_at(sent + i);
          int w = tcp_write(&a.tcp, client, chunk, n);
          if (w <= 0) break;
          sent += (size_t)w;
        }
        if (sent == total) {
          tcp_close(&a.tcp, client);
          closed = true;
        }
      }
    }

    if (server < 0) server = tcp_accept(&b.tcp, listener);
    if (server >= 0) {
      int r;
      while ((r = tcp_read(&b.tcp, server, chunk, sizeof(chunk))) > 0) {
        for (int i = 0; i < r; i++) same = same && chunk[i] == byte_at(got + (size_t)i);
        got += (size_t)r;
      }
      if (r < 0) eof = true;
    }
  }

  fprintf(stderr, "%zu of %zu bytes in %u ms, %s\n", got, total, sim_now(&wire),
          got == total && same ? "identical" : "NOT identical");
  fprintf(stderr, "retransmits %u, fast retransmits %u, timeouts %u\n", info.retransmits, info.fast_retransmits,
          info.timeouts);
  fprintf(stderr, "link: sent %u, lost %u, queue drops %u, reordered %u, duplicated %u, corrupted %u\n",
          wire.stats.sent, wire.stats.lost, wire.stats.queue_drops, wire.stats.reordered, wire.stats.duplicated,
          wire.stats.corrupted);
  return got == total && same ? 0 : 1;
}
