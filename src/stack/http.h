/*
 * A small HTTP/1.1 server, and the reason any of the rest exists. It answers one
 * request per connection and then closes, so there is no keep-alive to think
 * about. The fixed interface is every function below. The http_t struct is yours.
 *
 *   GET  /          a small HTML page
 *   GET  /bytes/N   N bytes of http_pattern(), so curl can check them
 *   POST /count     reads the body and answers with its length and FNV-1a hash
 */
#ifndef HTTP_H
#define HTTP_H

#include "tcp.h"

/** The longest request line and headers. Anything longer gets a 431. */
#define HTTP_MAX_HEADER 2048U

/** @brief One request in progress. */
typedef struct {
  int conn; // and everything else you need
} http_conn_t;

/** @brief The server. */
typedef struct {
  tcp_t *tcp; http_conn_t conns[TCP_MAX_CONNS]; // and everything else you need
} http_t;

/**
 * @brief The byte at an offset in a /bytes/N response.
 *
 * It looks random enough that a lost or repeated segment changes the hash, and
 * anyone can compute the expected file without asking the server.
 */
uint8_t http_pattern(uint32_t offset);

/**
 * @brief Start listening.
 * @return false if the port could not be opened.
 */
bool http_init(http_t *http, tcp_t *tcp, uint16_t port);

/** @brief Move every request along as far as it can go. It never blocks. */
void http_poll(http_t *http);

/** @brief How many requests have been answered. */
uint32_t http_served(const http_t *http);

#endif // HTTP_H
