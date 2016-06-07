#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>


#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)


#define MAKE_VALGRIND_HAPPY()                       \
  do {                                              \
    close_loop(uv_default_loop());                  \
    ASSERT(0 == uv_loop_close(uv_default_loop()));  \
  } while (0)

#if defined(__clang__) ||                                \
    defined(__GNUC__) ||                                 \
    defined(__INTEL_COMPILER) ||                         \
    defined(__SUNPRO_C)
# define UNUSED __attribute__((unused))
#else
# define UNUSED
#endif

#define NUM_CLIENTS 5
#define TRANSFER_BYTES (1 << 16)

#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b));


typedef enum {
  UNIDIRECTIONAL,
  DUPLEX
} test_mode_t;

typedef struct connection_context_s {
  uv_poll_t poll_handle;
  uv_timer_t timer_handle;
  uv_os_sock_t sock;
  size_t read, sent;
  int is_server_connection;
  int open_handles;
  int got_fin, sent_fin, got_disconnect;
  unsigned int events, delayed_events;
} connection_context_t;

typedef struct server_context_s {
  uv_poll_t poll_handle;
  uv_os_sock_t sock;
  int connections;
} server_context_t;

#define UNUSEDPARAM (void)


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void connection_poll_cb(uv_poll_t *handle, int status, int events);
extern void delay_timer_cb(uv_timer_t *timer);
extern int  got_eagain(void);
extern void close_socket(uv_os_sock_t sock);
extern void destroy_connection_context(connection_context_t *context);
extern void connection_close_cb(uv_handle_t *handle);

extern int valid_writable_wakeups;
extern int spurious_writable_wakeups;
extern int disconnects;
extern int closed_connections;

#ifdef __cplusplus
}
#endif //__cplusplus


#endif // UTILITY_H
