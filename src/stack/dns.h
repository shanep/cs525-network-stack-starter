/*
 * A DNS client for A records over UDP. It resolves one name at a time and never
 * blocks, so you start a lookup and then ask how it is going. The fixed
 * interface is every function below. The dns_t struct is yours.
 */
#ifndef DNS_H
#define DNS_H

#include "udp.h"

#define DNS_PORT 53
/** The port our queries come from. */
#define DNS_CLIENT_PORT 33533
/** RFC 1035's limit on a name as it goes on the wire. */
#define DNS_MAX_NAME 255U
#define DNS_RETRY_MS 1000U
#define DNS_MAX_TRIES 3U

typedef enum {
  DNS_IDLE,      /**< no lookup has been started */
  DNS_PENDING,   /**< the query is out and no answer has come back */
  DNS_DONE,      /**< got an address */
  DNS_NOT_FOUND, /**< the server says the name does not exist */
  DNS_FAILED     /**< no answer after DNS_MAX_TRIES, or the server gave an error */
} dns_status_t;

/** @brief All of the client's state. */
typedef struct {
  dns_status_t status; // and everything else you need
} dns_t;

/**
 * @brief Build a query for a name's A record.
 * @param buf Where the message goes.
 * @param cap The size of buf.
 * @param id The message ID.
 * @param name The name, like "www.example.com". A trailing dot is fine.
 * @return The message's length, or 0 if the name is not legal or it does not fit.
 */
size_t dns_build_query(uint8_t *buf, size_t cap, uint16_t id, const char *name);

/**
 * @brief Read a name out of a message, following compression pointers.
 *
 * A message comes from the network, so it can lie. Pointers that loop or that
 * point out of the message have to come back as an error and not hang the kernel.
 *
 * @param msg The whole message.
 * @param len The message's length.
 * @param off Where the name starts.
 * @param out Where the dotted name goes, nul terminated, with no trailing dot.
 * @param cap The size of out.
 * @param next Set to the offset just past the name where it first appeared.
 * @return false if the name is malformed or does not fit.
 */
bool dns_read_name(const uint8_t *msg, size_t len, size_t off, char *out, size_t cap, size_t *next);

/**
 * @brief Find the answer to a query in a response.
 * @param msg The whole message.
 * @param len The message's length.
 * @param id The ID the query used.
 * @param name The name the query asked about, which the response has to repeat.
 * @param addr Set to the first A record in the answer section, host order.
 * @return DNS_DONE, DNS_NOT_FOUND or DNS_FAILED, and DNS_PENDING if this message
 *         is not the answer to this query.
 */
dns_status_t dns_parse_response(const uint8_t *msg, size_t len, uint16_t id, const char *name, uint32_t *addr);

/**
 * @brief Start the client and bind its port.
 * @param dns The state to fill in.
 * @param udp UDP, already started.
 * @param server The DNS server's address, host order.
 * @param seed Where message IDs start.
 * @return false if the port could not be bound.
 */
bool dns_init(dns_t *dns, udp_t *udp, uint32_t server, uint16_t seed);

/**
 * @brief Start a lookup, which replaces the result of the last one.
 * @return false if one is already pending or the name is not legal.
 */
bool dns_resolve(dns_t *dns, const char *name);

/** @brief Tell the client what time it is, which retries a query that is due. */
void dns_tick(dns_t *dns, uint32_t now_ms);

/**
 * @brief See how the lookup is going.
 * @param dns The state.
 * @param addr Set to the address when the status is DNS_DONE.
 * @return The status.
 */
dns_status_t dns_result(const dns_t *dns, uint32_t *addr);

#endif // DNS_H
