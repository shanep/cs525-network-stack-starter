/*
 * The provided kernel services: serial console, timer, PCI and the handful of
 * C library functions the compiler and your stack expect to exist.
 *
 * Do NOT modify this file!
 */
#include "kernel.h"

#define COM1 0x3F8
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_HZ_PER_MS 1193U
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

void *memcpy(void *restrict d, const void *restrict s, size_t n) {
  void *start = d;
  __asm__ volatile("cld; rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
  return start;
}

void *memmove(void *d, const void *s, size_t n) {
  uint8_t *dp = d;
  const uint8_t *sp = s;
  if (dp < sp) {
    for (size_t i = 0; i < n; i++) dp[i] = sp[i];
  } else {
    for (size_t i = n; i > 0; i--) dp[i - 1] = sp[i - 1];
  }
  return d;
}

void *memset(void *d, int c, size_t n) {
  uint8_t *dp = d;
  for (size_t i = 0; i < n; i++) dp[i] = (uint8_t)c;
  return d;
}

int memcmp(const void *a, const void *b, size_t n) {
  const uint8_t *ap = a;
  const uint8_t *bp = b;
  for (size_t i = 0; i < n; i++) {
    if (ap[i] != bp[i]) return ap[i] - bp[i];
  }
  return 0;
}

size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n]) n++;
  return n;
}

void console_init(void) {
  outb(COM1 + 1, 0x00); // no interrupts
  outb(COM1 + 3, 0x80); // set the divisor
  outb(COM1 + 0, 0x01); // 115200 baud
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03); // 8 data bits, no parity, 1 stop bit
  outb(COM1 + 2, 0xC7); // turn on the FIFO
}

static void kputc(char c) {
  while (!(inb(COM1 + 5) & 0x20)) {
  }
  outb(COM1, (uint8_t)c);
}

void kputs(const char *s) {
  for (; *s; s++) {
    if (*s == '\n') kputc('\r');
    kputc(*s);
  }
}

void kputu(uint32_t n) {
  char buf[11];
  int i = 0;
  do {
    buf[i++] = (char)('0' + n % 10);
    n /= 10;
  } while (n);
  while (i > 0) kputc(buf[--i]);
}

void kputx(uint32_t n) {
  kputs("0x");
  for (int shift = 28; shift >= 0; shift -= 4) {
    kputc("0123456789abcdef"[(n >> shift) & 0xF]);
  }
}

void kexit(uint8_t code) {
  outl(0xF4, code);
  // Without the debug exit device on the QEMU command line we end up here.
  for (;;) __asm__ volatile("hlt");
}

void kpanic(const char *msg) {
  kputs("panic: ");
  kputs(msg);
  kputs("\n");
  kexit(1);
}

uint32_t timer_ms(void) {
  static bool started;
  static uint16_t last;
  static uint32_t leftover;
  static uint32_t ms;

  if (!started) {
    outb(PIT_COMMAND, 0x34); // channel 0, lo/hi byte, rate generator
    outb(PIT_CHANNEL0, 0x00);
    outb(PIT_CHANNEL0, 0x00); // reload of 65536
    started = true;
  }
  outb(PIT_COMMAND, 0x00); // latch channel 0
  uint16_t now = inb(PIT_CHANNEL0);
  now = (uint16_t)(now | (uint16_t)inb(PIT_CHANNEL0) << 8);

  // The counter counts down, and uint16_t arithmetic handles the wrap.
  leftover += (uint16_t)(last - now);
  last = now;
  ms += leftover / PIT_HZ_PER_MS;
  leftover %= PIT_HZ_PER_MS;
  return ms;
}

static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
  outl(PCI_ADDR, 0x80000000U | (uint32_t)bus << 16 | (uint32_t)slot << 11 | (uint32_t)func << 8 | (off & 0xFCU));
  return inl(PCI_DATA);
}

static void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint32_t value) {
  outl(PCI_ADDR, 0x80000000U | (uint32_t)bus << 16 | (uint32_t)slot << 11 | (uint32_t)func << 8 | (off & 0xFCU));
  outl(PCI_DATA, value);
}

bool pci_find(uint16_t vendor, uint16_t device, pci_device_t *dev) {
  uint32_t want = (uint32_t)device << 16 | vendor;
  // QEMU's default machine has one bus and no multi-function network cards.
  for (uint8_t slot = 0; slot < 32; slot++) {
    if (pci_read(0, slot, 0, 0) == want) {
      dev->bus = 0;
      dev->slot = slot;
      dev->func = 0;
      dev->bar0 = pci_read(0, slot, 0, 0x10);
      return true;
    }
  }
  return false;
}

void pci_enable(const pci_device_t *dev) {
  uint32_t command = pci_read(dev->bus, dev->slot, dev->func, 0x04);
  // bit 0 is port I/O, bit 2 is bus master, which is what allows DMA
  pci_write(dev->bus, dev->slot, dev->func, 0x04, command | 0x5);
}
