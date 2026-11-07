/*
 * TCP. It sits on nothing but a function pointer, so you can test all of it
 * over the simulator before IP or Ethernet exist. The fixed interface is every
 * function below. The tcb_t struct is yours, so add whatever you need to it.
 */
#ifndef TCP_H
#define TCP_H

#include "port.h"

/** The IP protocol number for TCP. */
#define TCP_PROTO 6
/** How many connections one tcp_t holds, listeners do not count. */
#define TCP_MAX_CONNS 8
/** How many ports one tcp_t can listen on. */
#define TCP_MAX_LISTEN 4
/** The send and receive buffers. A power of two, so sequence numbers index it. */
#define TCP_BUF_SIZE 65536U
/** Bigger than this does not fit in an Ethernet frame with no options. */
#define TCP_MAX_MSS 1460U
/** RFC 9293's default when the peer sends no MSS option. */
#define TCP_DEFAULT_MSS 536U
/** RFC 6298 starting RTO. */
#define TCP_RTO_INIT_MS 1000U
/** The floor on the RTO. RFC 6298 says 1 second, Linux uses 200 ms and so do we. */
#define TCP_RTO_MIN_MS 200U
#define TCP_RTO_MAX_MS 60000U
/** Give up and abort after this many timeouts on the same data. */
#define TCP_MAX_RETRIES 12U
/** TIME_WAIT lasts 2 MSL. Linux uses 60 seconds in total, 10 is plenty for us. */
#define TCP_MSL_MS 5000U
/** Give up on a peer that never sends its FIN after we closed. */
#define TCP_FIN_WAIT_2_MS 60000U

typedef enum {
  TCP_CLOSED,
  TCP_LISTEN,
  TCP_SYN_SENT,
  TCP_SYN_RCVD,
  TCP_ESTABLISHED,
  TCP_FIN_WAIT_1,
  TCP_FIN_WAIT_2,
  TCP_CLOSE_WAIT,
  TCP_CLOSING,
  TCP_LAST_ACK,
  TCP_TIME_WAIT
} tcp_state_t;

/**
 * @brief Where finished segments go, IP in the real stack and the simulator in
 * your first tests.
 * @param ctx The context you gave tcp_init().
 * @param src Source address, host order.
 * @param dst Destination address, host order.
 * @param segment The whole TCP segment, header first, checksum filled in. It is
 *        only valid until the call returns.
 * @param len The segment's length.
 */
typedef void (*tcp_output_fn)(void *ctx, uint32_t src, uint32_t dst, const uint8_t *segment, size_t len);

/** @brief A look inside a connection, for tests and for plotting cwnd. */
typedef struct {
  tcp_state_t state;
  uint32_t cwnd;
  uint32_t ssthresh;
  uint32_t snd_wnd;
  uint32_t mss;
  uint32_t rto_ms;
  uint32_t srtt_ms;       /**< 0 until the first sample */
  uint32_t flight;        /**< bytes sent and not yet acknowledged */
  uint32_t retransmits;   /**< every segment sent a second time, for any reason */
  uint32_t fast_retransmits;
  uint32_t timeouts;
  bool in_recovery;
} tcp_info_t;

/** @brief One connection. */
typedef struct {
  tcp_state_t state; // and everything else you need
} tcb_t;

/** @brief All of TCP's state. Over a megabyte, so make it static. */
typedef struct {
  tcb_t conns[TCP_MAX_CONNS]; // and everything else you need
} tcp_t;

/**
 * @brief Start TCP with no connections.
 * @param tcp The state to fill in.
 * @param output Where finished segments go.
 * @param ctx Handed back to output on every call.
 * @param addr This host's address, host order.
 * @param isn The first connection's initial sequence number, and each new
 *        connection gets a different one. A test that fixes this can check
 *        exact sequence numbers.
 */
void tcp_init(tcp_t *tcp, tcp_output_fn output, void *ctx, uint32_t addr, uint32_t isn);

/**
 * @brief Hand TCP one segment that came up from the layer below.
 *
 * A segment with a bad checksum, or too short to hold its header, is dropped
 * with no reply. A segment for no connection gets a RST, unless it is a RST.
 *
 * @param tcp The state.
 * @param src Source address, host order.
 * @param dst Destination address, host order.
 * @param segment The segment, which is only valid until the call returns.
 * @param len The segment's length.
 */
void tcp_input(tcp_t *tcp, uint32_t src, uint32_t dst, const uint8_t *segment, size_t len);

/**
 * @brief Tell TCP what time it is and fire any timers that are due.
 * @param tcp The state.
 * @param now_ms The time, which wraps at 2^32.
 */
void tcp_tick(tcp_t *tcp, uint32_t now_ms);

/**
 * @brief Start listening on a port.
 * @return A listener handle, or -1 if the port is 0 or in use or there is no room.
 */
int tcp_listen(tcp_t *tcp, uint16_t port);

/**
 * @brief Take a connection that finished its handshake on this listener.
 * @return A connection handle, or -1 if none is waiting. This never blocks.
 */
int tcp_accept(tcp_t *tcp, int listener);

/**
 * @brief Start an active open. This returns right away, so poll tcp_get_info().
 * @return A connection handle, or -1 if there is no room.
 */
int tcp_connect(tcp_t *tcp, uint32_t dst, uint16_t port);

/**
 * @brief Queue bytes to send. Bytes queued during the handshake go out when it
 * finishes.
 * @return How many bytes fit in the send buffer, which can be 0, or -1 if the
 *         connection can not send (bad handle, closed, or tcp_close() was called).
 */
int tcp_write(tcp_t *tcp, int conn, const void *data, size_t len);

/**
 * @brief Take bytes that have arrived in order.
 * @return How many bytes were copied. 0 means nothing yet but more could come,
 *         and -1 means the stream is over (peer closed or reset, or a bad handle).
 */
int tcp_read(tcp_t *tcp, int conn, void *data, size_t len);

/**
 * @brief Send a FIN after everything already queued, and give up the handle.
 *
 * Do not use the handle after this. TCP finishes the close on its own and frees
 * the connection when it is done. Closing before the handshake finishes drops
 * the connection and anything queued on it, which is what RFC 9293 says to do.
 */
void tcp_close(tcp_t *tcp, int conn);

/**
 * @brief Look inside a connection.
 * @return false if the handle is not a live connection.
 */
bool tcp_get_info(const tcp_t *tcp, int conn, tcp_info_t *info);

#endif // TCP_H
