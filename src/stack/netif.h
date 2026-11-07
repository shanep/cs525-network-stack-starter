/*
 * The line between your stack and the thing that moves bytes. The stack only
 * ever talks to one of these, so the same code runs over the simulator under
 * your tests and over your virtio-net driver in the kernel.
 *
 * Do NOT modify this file!
 */
#ifndef NETIF_H
#define NETIF_H

#include "port.h"

typedef struct {
  /** Hand one Ethernet frame to the wire. Copies the frame before returning. */
  void (*tx)(void *ctx, const uint8_t *frame, size_t len);
  /** Take one frame off the wire. Returns its length, 0 when nothing is waiting. */
  size_t (*rx)(void *ctx, uint8_t *frame, size_t cap);
  /** Milliseconds from a clock that only moves forward. It wraps at 2^32. */
  uint32_t (*now_ms)(void *ctx);
  void *ctx;
} netif_t;

#endif // NETIF_H
