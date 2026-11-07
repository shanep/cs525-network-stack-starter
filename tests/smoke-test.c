/*
 * One small test per milestone, using nothing but the fixed interface. Each one fails
 * in the starter and passes once that milestone works, and MILESTONE in main-test.c
 * says how far along to run. They are not enough on their own, since they barely
 * touch timers or bad input, so write your own tests in this directory.
 */
#include <string.h>

#include "harness/unity.h"
#include "http.h"
#include "sim.h"
#include "stack.h"

#define ADDR_A IP4(10, 0, 0, 1)
#define ADDR_B IP4(10, 0, 0, 2)
#define SMOKE_PORT 9000
#define HTTP_PORT 80

static sim_link_t wire;

/* ---- G0: the checksum everything else needs ---- */

void test_smoke_g0_checksum_rfc1071_example(void) {
  // The worked example in RFC 1071, section 3.
  const uint8_t data[] = {0x00, 0x01, 0xF2, 0x03, 0xF4, 0xF5, 0xF6, 0xF7};
  TEST_ASSERT_EQUAL_HEX16(0x220D, net_checksum_finish(net_checksum_add(0, data, sizeof(data))));
}

/* ---- G1: TCP with nothing under it but the simulator ---- */

static tcp_t tcp_a, tcp_b;

static void tcp_out_a(void *ctx, uint32_t src, uint32_t dst, const uint8_t *seg, size_t len) {
  (void)ctx;
  (void)src;
  (void)dst;
  sim_send(&wire, 0, seg, len);
}

static void tcp_out_b(void *ctx, uint32_t src, uint32_t dst, const uint8_t *seg, size_t len) {
  (void)ctx;
  (void)src;
  (void)dst;
  sim_send(&wire, 1, seg, len);
}

static void tcp_pump(void) {
  static uint8_t seg[SIM_MAX_PACKET];
  size_t n;
  sim_advance(&wire, 1);
  while ((n = sim_recv(&wire, 0, seg, sizeof(seg))) > 0) tcp_input(&tcp_a, ADDR_B, ADDR_A, seg, n);
  while ((n = sim_recv(&wire, 1, seg, sizeof(seg))) > 0) tcp_input(&tcp_b, ADDR_A, ADDR_B, seg, n);
  tcp_tick(&tcp_a, sim_now(&wire));
  tcp_tick(&tcp_b, sim_now(&wire));
}

void test_smoke_g1_tcp_moves_bytes_past_loss(void) {
  sim_config_t link = {.delay_ms = 10, .loss = 0.02, .seed = 7};
  sim_init(&wire, &link);
  tcp_init(&tcp_a, tcp_out_a, NULL, ADDR_A, 1000);
  tcp_init(&tcp_b, tcp_out_b, NULL, ADDR_B, 900000);
  int listener = tcp_listen(&tcp_b, SMOKE_PORT);
  TEST_ASSERT_TRUE(listener >= 0);
  int client = tcp_connect(&tcp_a, ADDR_B, SMOKE_PORT);
  TEST_ASSERT_TRUE(client >= 0);

  static uint8_t want[100000], got[100000];
  for (size_t i = 0; i < sizeof(want); i++) want[i] = (uint8_t)(i * 31U);
  size_t sent = 0, have = 0;
  int server = -1;
  for (int ms = 0; ms < 120000 && have < sizeof(got); ms++) {
    tcp_pump();
    if (server < 0) server = tcp_accept(&tcp_b, listener);
    int r = tcp_write(&tcp_a, client, want + sent, sizeof(want) - sent);
    if (r > 0) sent += (size_t)r;
    if (server >= 0) {
      r = tcp_read(&tcp_b, server, got + have, sizeof(got) - have);
      if (r > 0) have += (size_t)r;
    }
  }
  TEST_ASSERT_EQUAL_size_t(sizeof(want), have);
  TEST_ASSERT_EQUAL_MEMORY(want, got, sizeof(want));
}

/* ---- G2: IP's routing, and an echo that goes out and comes back ---- */

static ip_t ip_a, ip_b;

static void ip_out_a(void *ctx, uint32_t next_hop, const uint8_t *packet, size_t len) {
  (void)ctx;
  (void)next_hop;
  ip_input(&ip_b, packet, len);
}

static void ip_out_b(void *ctx, uint32_t next_hop, const uint8_t *packet, size_t len) {
  (void)ctx;
  (void)next_hop;
  ip_input(&ip_a, packet, len);
}

void test_smoke_g2_longest_prefix_and_ping(void) {
  ip_init(&ip_a, ip_out_a, NULL, ADDR_A);
  ip_init(&ip_b, ip_out_b, NULL, ADDR_B);
  TEST_ASSERT_TRUE(ip_route_add(&ip_a, 0, 0, IP4(10, 0, 0, 254)));
  TEST_ASSERT_TRUE(ip_route_add(&ip_a, IP4(10, 0, 0, 0), 24, 0));
  TEST_ASSERT_TRUE(ip_route_add(&ip_b, IP4(10, 0, 0, 0), 24, 0));

  uint32_t next_hop = 0;
  TEST_ASSERT_TRUE(ip_route_lookup(&ip_a, ADDR_B, &next_hop));
  TEST_ASSERT_EQUAL_HEX32(ADDR_B, next_hop);
  TEST_ASSERT_TRUE(ip_route_lookup(&ip_a, IP4(8, 8, 8, 8), &next_hop));
  TEST_ASSERT_EQUAL_HEX32(IP4(10, 0, 0, 254), next_hop);

  TEST_ASSERT_TRUE(ip_ping(&ip_a, ADDR_B, 1, 1));
  TEST_ASSERT_EQUAL_UINT32(1, ip_ping_replies(&ip_a));
}

/* ---- G3 and G4: the whole stack on both ends of the simulator ---- */

static sim_port_t port_a, port_b;
static net_stack_t a, b;
static http_t server;

static void stack_start(void) {
  sim_config_t link = {.delay_ms = 5};
  sim_init(&wire, &link);
  netif_t netif_a = sim_netif(&port_a, &wire, 0);
  netif_t netif_b = sim_netif(&port_b, &wire, 1);
  net_config_t ca = {.mac = {2, 0, 0, 0, 0, 1}, .addr = ADDR_A, .prefix_len = 24, .isn = 1000};
  net_config_t cb = {.mac = {2, 0, 0, 0, 0, 2}, .addr = ADDR_B, .prefix_len = 24, .isn = 900000};
  net_init(&a, &netif_a, &ca);
  net_init(&b, &netif_b, &cb);
}

void test_smoke_g3_ping_finds_the_hardware_address(void) {
  stack_start();
  // ARP holds on to this ping and lets it go when the answer comes back.
  ip_ping(&a.ip, ADDR_B, 1, 1);
  for (int ms = 0; ms < 100; ms++) {
    sim_advance(&wire, 1);
    net_poll(&a);
    net_poll(&b);
  }
  TEST_ASSERT_EQUAL_UINT32(1, ip_ping_replies(&a.ip));
  uint8_t mac[6];
  TEST_ASSERT_TRUE(arp_lookup(&a.eth, ADDR_B, mac));
}

void test_smoke_g4_http_answers_a_request(void) {
  stack_start();
  TEST_ASSERT_TRUE(http_init(&server, &b.tcp, HTTP_PORT));
  int conn = tcp_connect(&a.tcp, ADDR_B, HTTP_PORT);
  TEST_ASSERT_TRUE(conn >= 0);
  const char *request = "GET /bytes/5000 HTTP/1.1\r\nHost: smoke\r\n\r\n";
  TEST_ASSERT_EQUAL_INT((int)strlen(request), tcp_write(&a.tcp, conn, request, strlen(request)));

  static char got[8000];
  size_t have = 0;
  int r = 0;
  for (int ms = 0; ms < 60000 && r >= 0; ms++) {
    sim_advance(&wire, 1);
    net_poll(&a);
    net_poll(&b);
    http_poll(&server);
    r = tcp_read(&a.tcp, conn, got + have, sizeof(got) - have);
    if (r > 0) have += (size_t)r;
  }
  TEST_ASSERT_TRUE_MESSAGE(r < 0, "the server never closed the connection");
  TEST_ASSERT_EQUAL_INT(0, memcmp(got, "HTTP/1.1 200", 12));
  char *body = NULL;
  for (size_t i = 0; i + 4 <= have; i++) {
    if (memcmp(got + i, "\r\n\r\n", 4) == 0) {
      body = got + i + 4;
      break;
    }
  }
  TEST_ASSERT_NOT_NULL(body);
  TEST_ASSERT_EQUAL_size_t(5000, have - (size_t)(body - got));
  for (uint32_t i = 0; i < 5000; i++) TEST_ASSERT_EQUAL_HEX8(http_pattern(i), (uint8_t)body[i]);
}

void run_smoke_tests(int milestone) {
  RUN_TEST(test_smoke_g0_checksum_rfc1071_example);
  if (milestone >= 1) RUN_TEST(test_smoke_g1_tcp_moves_bytes_past_loss);
  if (milestone >= 2) RUN_TEST(test_smoke_g2_longest_prefix_and_ping);
  if (milestone >= 3) RUN_TEST(test_smoke_g3_ping_finds_the_hardware_address);
  if (milestone >= 4) RUN_TEST(test_smoke_g4_http_answers_a_request);
}
