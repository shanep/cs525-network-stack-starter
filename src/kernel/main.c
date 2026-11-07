/*
 * The unikernel. Find the card, start the stack, then spin forever: one loop, no
 * interrupts and no threads. QEMU's user networking puts us at 10.0.2.15 with the
 * gateway at 10.0.2.2 and DNS at 10.0.2.3.
 *
 * Do NOT modify this file!
 */
#include "http.h"
#include "kernel.h"
#include "stack.h"
#include "virtio_net.h"

#define HTTP_PORT 80
#define GATEWAY IP4(10, 0, 2, 2)
#define BOOT_NAME "example.com"
#define BOOT_CHECK_MS 3000U
#define PING_EVERY_MS 500U

static net_stack_t net;
static http_t http;
static virtio_net_t nic;

/* An initial sequence number nobody can guess needs a real source of randomness,
   and this is not one. RFC 9293 asks for better. It does differ on every boot. */
static uint32_t boot_entropy(void) {
  uint32_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return (low ^ high) * 2654435761U;
}

static void put_ip(uint32_t addr) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    kputu((addr >> shift) & 0xFFU);
    if (shift) kputs(".");
  }
}

/* These only show that ICMP and DNS work, so a failure is just a message. A
   machine with no way out to the Internet still has to serve pages. */
static void boot_checks(void) {
  dns_resolve(&net.dns, BOOT_NAME);
  uint32_t start = timer_ms();
  uint32_t next_ping = start;
  uint16_t seq = 1;
  bool pinged = false;
  dns_status_t status = DNS_PENDING;
  uint32_t addr = 0;
  while (timer_ms() - start < BOOT_CHECK_MS && (!pinged || status == DNS_PENDING)) {
    net_poll(&net);
    // The first ping only starts ARP, so keep asking until one comes back.
    if (!pinged && (int32_t)(timer_ms() - next_ping) >= 0) {
      ip_ping(&net.ip, GATEWAY, 1, seq++);
      next_ping += PING_EVERY_MS;
    }
    pinged = ip_ping_replies(&net.ip) > 0;
    status = dns_result(&net.dns, &addr);
  }
  kputs(pinged ? "ping gateway: ok\n" : "ping gateway: no answer\n");
  if (status == DNS_DONE) {
    kputs("dns " BOOT_NAME ": ");
    put_ip(addr);
    kputs("\n");
  } else {
    kputs("dns " BOOT_NAME ": no answer\n");
  }
}

void kmain(void) {
  console_init();
  kputs("hello from the unikernel\n");

  pci_device_t dev;
  if (!pci_find(VIRTIO_VENDOR, VIRTIO_NET_DEVICE, &dev)) kpanic("no virtio-net card");
  if (!virtio_net_init(&nic, &dev)) kpanic("virtio-net would not start");

  net_config_t config = {
      .addr = IP4(10, 0, 2, 15),
      .prefix_len = 24,
      .gateway = GATEWAY,
      .dns = IP4(10, 0, 2, 3),
      .isn = boot_entropy(),
  };
  virtio_net_mac(&nic, config.mac);
  netif_t netif = virtio_net_netif(&nic);
  net_init(&net, &netif, &config);
  kputs("stack up at ");
  put_ip(config.addr);
  kputs("\n");

  boot_checks();

  if (!http_init(&http, &net.tcp, HTTP_PORT)) kpanic("http would not start");
  kputs("http listening on port 80\n");
  for (;;) {
    net_poll(&net);
    http_poll(&http);
  }
}
