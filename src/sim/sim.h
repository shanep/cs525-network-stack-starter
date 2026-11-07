/*
 * A network in a box. A sim_link_t is a cable with two ends that delivers
 * packets after a delay, at a limited rate, through a drop-tail queue, and can
 * lose, duplicate, reorder and corrupt them at the rates you ask for. The same
 * seed gives the same damage, so a test that fails will fail the same way again.
 *
 * This code only runs on Linux, under your tests. The clock is yours: nothing
 * happens until you call sim_advance(), so a test that covers a 60 second
 * timeout takes no time to run. Do NOT modify this file!
 */
#ifndef SIM_H
#define SIM_H

#include "netif.h"
#include "port.h"

#define SIM_SLOTS 512
#define SIM_MAX_PACKET 1600

/** @brief How a link behaves. Rates are from 0.0 (never) to 1.0 (always). */
typedef struct {
  uint32_t delay_ms;           /**< one way propagation delay */
  uint32_t bytes_per_ms;       /**< bottleneck rate, 0 for unlimited */
  uint32_t queue_packets;      /**< drop-tail limit, 0 for SIM_SLOTS */
  double loss;                 /**< lose a packet */
  double duplicate;            /**< deliver a packet twice */
  double reorder;              /**< hold a packet back for reorder_ms */
  uint32_t reorder_ms;         /**< how long a reordered packet is held */
  double corrupt;              /**< flip one bit of a packet */
  uint32_t seed;               /**< the same seed gives the same damage */
} sim_config_t;

/** @brief What happened on a link. */
typedef struct {
  uint32_t sent;
  uint32_t delivered;
  uint32_t lost;
  uint32_t queue_drops;
  uint32_t duplicated;
  uint32_t reordered;
  uint32_t corrupted;
} sim_stats_t;

typedef struct {
  uint32_t deliver_at;
  uint32_t seq;
  size_t len;
  bool used;
  uint8_t data[SIM_MAX_PACKET];
} sim_slot_t;

/** @brief A cable with two ends, 0 and 1. Big, so make it static. */
typedef struct {
  sim_config_t config;
  sim_stats_t stats;
  uint32_t now;
  uint32_t rng;
  uint32_t next_seq;
  uint32_t busy_until[2];
  sim_slot_t slots[2][SIM_SLOTS]; /**< slots[n] is what is on its way to end n */
} sim_link_t;

/** @brief Start a link at time 0. */
void sim_init(sim_link_t *link, const sim_config_t *config);
/** @brief Change the damage part way through, keeping the packets in flight. */
void sim_set_config(sim_link_t *link, const sim_config_t *config);
/** @brief Move the clock forward. */
void sim_advance(sim_link_t *link, uint32_t ms);
/** @brief The current time on this link's clock. */
uint32_t sim_now(const sim_link_t *link);
/** @brief Send a packet from one end of the link toward the other end. */
void sim_send(sim_link_t *link, int from_end, const uint8_t *data, size_t len);
/**
 * @brief Take the oldest packet that has arrived at this end.
 * @return The packet's length, or 0 if nothing has arrived. A packet bigger
 *         than cap is truncated.
 */
size_t sim_recv(sim_link_t *link, int end, uint8_t *data, size_t cap);

/** @brief One end of a link as a network interface, for the whole stack tests. */
typedef struct {
  sim_link_t *link;
  int end;
} sim_port_t;

/**
 * @brief Make a netif_t out of one end of a link.
 * @param port Storage for the adapter, which has to outlive the netif.
 * @param link The link.
 * @param end Which end, 0 or 1.
 * @return The interface to hand to net_init().
 */
netif_t sim_netif(sim_port_t *port, sim_link_t *link, int end);

#endif // SIM_H
