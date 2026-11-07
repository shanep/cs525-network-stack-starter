/*
 * Everything the unikernel provides. There is no C library, no scheduler, no
 * interrupts and no virtual memory, just this and one polling loop.
 *
 * Do NOT modify this file!
 */
#ifndef KERNEL_H
#define KERNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* x86 port I/O */
static inline void outb(uint16_t port, uint8_t v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
static inline void outw(uint16_t port, uint16_t v) { __asm__ volatile("outw %0, %1" : : "a"(v), "Nd"(port)); }
static inline void outl(uint16_t port, uint32_t v) { __asm__ volatile("outl %0, %1" : : "a"(v), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) {
  uint8_t v;
  __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
  return v;
}
static inline uint16_t inw(uint16_t port) {
  uint16_t v;
  __asm__ volatile("inw %1, %0" : "=a"(v) : "Nd"(port));
  return v;
}
static inline uint32_t inl(uint16_t port) {
  uint32_t v;
  __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
  return v;
}

/** @brief Stop the compiler moving memory accesses across this line. */
#define barrier() __asm__ volatile("" : : : "memory")

/** @brief Setup COM1, which QEMU's -nographic connects to your terminal. */
void console_init(void);
/** @brief Write a string to the serial console. */
void kputs(const char *s);
/** @brief Write a number in decimal. */
void kputu(uint32_t n);
/** @brief Write a number in hexadecimal with a 0x in front. */
void kputx(uint32_t n);
/** @brief Print a message and halt, never returns. */
void kpanic(const char *msg);

/**
 * @brief Milliseconds since the first call.
 *
 * This polls the PIT, whose counter wraps every 54 ms, so it must be called
 * more often than that. The main loop does it on every pass.
 */
uint32_t timer_ms(void);

/** @brief The address and interrupt-free facts we need about a PCI device. */
typedef struct {
  uint8_t bus;
  uint8_t slot;
  uint8_t func;
  uint32_t bar0;
} pci_device_t;

/**
 * @brief Find the first PCI device with this vendor and device id.
 * @return true and fills in dev if the device is on the bus.
 */
bool pci_find(uint16_t vendor, uint16_t device, pci_device_t *dev);
/** @brief Turn on port I/O and DMA for a device. */
void pci_enable(const pci_device_t *dev);

/**
 * @brief Leave QEMU. Needs -device isa-debug-exit,iobase=0xf4,iosize=0x04.
 * QEMU's own exit status is (code << 1) | 1.
 */
void kexit(uint8_t code);

#endif // KERNEL_H
