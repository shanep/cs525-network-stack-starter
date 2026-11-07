// Do NOT modify this file!
#include "sim.h"

static uint32_t next_random(sim_link_t *link) {
  // xorshift32. Not a good generator, but a good one is not the point here.
  uint32_t x = link->rng;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  link->rng = x;
  return x;
}

static bool chance(sim_link_t *link, double rate) {
  if (rate <= 0.0) return false;
  return (double)next_random(link) / 4294967296.0 < rate;
}

static bool expired(uint32_t now, uint32_t when) { return (int32_t)(now - when) >= 0; }

void sim_init(sim_link_t *link, const sim_config_t *config) {
  memset(link, 0, sizeof(*link));
  sim_set_config(link, config);
  // xorshift turns a small state into a small first number, which would lose the
  // first packet of every run with a seed of 1, 2 or 3. So mix the seed first.
  uint32_t mixed = (config->seed ? config->seed : 1) * 2654435769U;
  mixed ^= mixed >> 16;
  link->rng = mixed ? mixed : 1;
}

void sim_set_config(sim_link_t *link, const sim_config_t *config) { link->config = *config; }

void sim_advance(sim_link_t *link, uint32_t ms) { link->now += ms; }

uint32_t sim_now(const sim_link_t *link) { return link->now; }

static void enqueue(sim_link_t *link, int to, const uint8_t *data, size_t len, uint32_t deliver_at) {
  sim_slot_t *slots = link->slots[to];
  uint32_t limit = link->config.queue_packets ? link->config.queue_packets : SIM_SLOTS;
  if (limit > SIM_SLOTS) limit = SIM_SLOTS;
  uint32_t in_use = 0;
  sim_slot_t *free_slot = NULL;
  for (size_t i = 0; i < SIM_SLOTS; i++) {
    if (slots[i].used) {
      in_use++;
    } else if (!free_slot) {
      free_slot = &slots[i];
    }
  }
  if (in_use >= limit || !free_slot) {
    link->stats.queue_drops++;
    return;
  }
  if (len > SIM_MAX_PACKET) len = SIM_MAX_PACKET;
  memcpy(free_slot->data, data, len);
  free_slot->len = len;
  free_slot->deliver_at = deliver_at;
  free_slot->seq = link->next_seq++;
  free_slot->used = true;
}

void sim_send(sim_link_t *link, int from_end, const uint8_t *data, size_t len) {
  int to = 1 - from_end;
  link->stats.sent++;

  // A packet the queue will drop never takes up time on the wire.
  uint32_t limit = link->config.queue_packets ? link->config.queue_packets : SIM_SLOTS;
  uint32_t in_use = 0;
  for (size_t i = 0; i < SIM_SLOTS; i++) in_use += link->slots[to][i].used;
  bool will_fit = in_use < limit && in_use < SIM_SLOTS;

  uint32_t start = link->now;
  if (expired(link->busy_until[to], start)) start = link->busy_until[to];
  uint32_t on_wire = 0;
  if (link->config.bytes_per_ms) {
    on_wire = (uint32_t)(len + link->config.bytes_per_ms - 1) / link->config.bytes_per_ms;
  }
  uint32_t finish = start + on_wire;
  if (will_fit) link->busy_until[to] = finish;

  // A lost packet still used the bottleneck, so the loss comes after the queue.
  if (chance(link, link->config.loss)) {
    link->stats.lost++;
    return;
  }

  uint8_t copy[SIM_MAX_PACKET];
  size_t n = len > SIM_MAX_PACKET ? SIM_MAX_PACKET : len;
  memcpy(copy, data, n);
  if (n > 0 && chance(link, link->config.corrupt)) {
    uint32_t bit = next_random(link) % (uint32_t)(n * 8);
    copy[bit / 8] ^= (uint8_t)(1U << (bit % 8));
    link->stats.corrupted++;
  }

  uint32_t at = finish + link->config.delay_ms;
  if (chance(link, link->config.reorder)) {
    at += link->config.reorder_ms;
    link->stats.reordered++;
  }
  enqueue(link, to, copy, n, at);
  if (chance(link, link->config.duplicate)) {
    link->stats.duplicated++;
    enqueue(link, to, copy, n, at + 1);
  }
}

size_t sim_recv(sim_link_t *link, int end, uint8_t *data, size_t cap) {
  sim_slot_t *slots = link->slots[end];
  sim_slot_t *best = NULL;
  for (size_t i = 0; i < SIM_SLOTS; i++) {
    sim_slot_t *s = &slots[i];
    if (!s->used || !expired(link->now, s->deliver_at)) continue;
    if (!best || (int32_t)(s->deliver_at - best->deliver_at) < 0 ||
        (s->deliver_at == best->deliver_at && (int32_t)(s->seq - best->seq) < 0)) {
      best = s;
    }
  }
  if (!best) return 0;
  size_t n = best->len > cap ? cap : best->len;
  memcpy(data, best->data, n);
  best->used = false;
  link->stats.delivered++;
  return n;
}

static void port_tx(void *ctx, const uint8_t *frame, size_t len) {
  sim_port_t *port = ctx;
  sim_send(port->link, port->end, frame, len);
}

static size_t port_rx(void *ctx, uint8_t *frame, size_t cap) {
  sim_port_t *port = ctx;
  return sim_recv(port->link, port->end, frame, cap);
}

static uint32_t port_now(void *ctx) {
  sim_port_t *port = ctx;
  return sim_now(port->link);
}

netif_t sim_netif(sim_port_t *port, sim_link_t *link, int end) {
  port->link = link;
  port->end = end;
  netif_t netif = {port_tx, port_rx, port_now, port};
  return netif;
}
