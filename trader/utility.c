#include "utility.h"

int closed_connections = 0;
int disconnects = 0;
int valid_writable_wakeups    = 0;
int spurious_writable_wakeups = 0;



void delay_timer_cb(uv_timer_t *timer) {
  connection_context_t *context = (connection_context_t *)timer->data;
  int r;

  /* Timer should auto stop. */
  ASSERT(0 == uv_is_active((uv_handle_t *)timer));

  /* Add the requested events to the poll mask. */
  ASSERT(context->delayed_events != 0);
  context->events |= context->delayed_events;
  context->delayed_events = 0;

  r = uv_poll_start(&context->poll_handle, context->events, connection_poll_cb);
  ASSERT(r == 0);
}

int got_eagain(void) {
  return errno == EAGAIN || errno == EINPROGRESS || errno == EWOULDBLOCK;
}

void close_socket(uv_os_sock_t sock) {
  int r;
  r = close(sock);
  ASSERT(r == 0);
}


void destroy_connection_context(connection_context_t *context) {
  uv_close((uv_handle_t *)&context->poll_handle, connection_close_cb);
  uv_close((uv_handle_t *)&context->timer_handle, connection_close_cb);
}

void connection_close_cb(uv_handle_t *handle) {
  connection_context_t *context = (connection_context_t *)handle->data;

  if (--context->open_handles == 0) {
    if (context->is_server_connection) {
      ASSERT(context->read == TRANSFER_BYTES);
    } else {
      ASSERT(context->read == 0);
    }

    if (!context->is_server_connection) {
      ASSERT(context->sent == TRANSFER_BYTES);
    } else {
      ASSERT(context->sent == 0);
    }

    closed_connections++;

    free(context);
  }
}
