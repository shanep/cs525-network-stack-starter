/*
 * What every layer of the stack shares. This code has to compile with no C
 * library for the kernel, so it can only include the freestanding headers, and
 * these prototypes stand in for <string.h>. On Linux the C library provides the
 * functions, and in the kernel src/kernel/kernel.c does.
 *
 * Do NOT modify this file!
 */
#ifndef PORT_H
#define PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);

/** @brief Read a big endian (network order) 16 bit value from a packet. */
static inline uint16_t rd16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }

/** @brief Read a big endian (network order) 32 bit value from a packet. */
static inline uint32_t rd32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/** @brief Write a 16 bit value into a packet in network order. */
static inline void wr16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}

/** @brief Write a 32 bit value into a packet in network order. */
static inline void wr32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

/** @brief Build the host order address a.b.c.d. */
#define IP4(a, b, c, d) ((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c) << 8 | (uint32_t)(d))

/**
 * @brief Add a buffer to a running Internet checksum (RFC 1071).
 *
 * The sum is built up in pieces, because TCP and UDP checksum a pseudo-header
 * and then the segment. Every piece except the last must have an even length.
 *
 * @param sum The running sum, 0 to start.
 * @param data The bytes to add.
 * @param len How many bytes.
 * @return The new running sum, which you hand to net_checksum_finish().
 */
uint32_t net_checksum_add(uint32_t sum, const void *data, size_t len);

/** @brief Fold a running sum into 16 bits and take the one's complement. */
uint16_t net_checksum_finish(uint32_t sum);

/**
 * @brief Add the IPv4 pseudo-header that TCP and UDP checksums cover.
 * @param sum The running sum.
 * @param src Source address, host order.
 * @param dst Destination address, host order.
 * @param proto The IP protocol number.
 * @param len The length of the TCP or UDP segment.
 * @return The new running sum.
 */
uint32_t net_checksum_pseudo(uint32_t sum, uint32_t src, uint32_t dst, uint8_t proto, uint16_t len);

#endif // PORT_H
