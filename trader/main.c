#include "trader.h"

static short TEST_PORT = 12345;

/* Fully close a loop */
static void close_walk_cb(uv_handle_t *handle, void *arg) {
  UNUSEDPARAM arg;
  if (!uv_is_closing(handle))
    uv_close(handle, NULL);
}

UNUSED static void close_loop(uv_loop_t *loop) {
  uv_walk(loop, close_walk_cb, NULL);
  uv_run(loop, UV_RUN_DEFAULT);
}

static void delay_timer_cb(uv_timer_t *timer);

static test_mode_t test_mode = DUPLEX;

static int closed_connections = 0;

static int valid_writable_wakeups = 0;
static int spurious_writable_wakeups = 0;
static int disconnects = 0;

static int got_eagain(void) {
  return errno == EAGAIN || errno == EINPROGRESS || errno == EWOULDBLOCK;
}

static uv_os_sock_t create_bound_socket(struct sockaddr_in bind_addr) {
  uv_os_sock_t sock;
  int r;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  ASSERT(sock >= 0);

  {
    /* Allow reuse of the port. */
    int yes = 1;
    r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    ASSERT(r == 0);
  }

  r = bind(sock, (const struct sockaddr *)&bind_addr, sizeof bind_addr);
  ASSERT(r == 0);

  return sock;
}

static void close_socket(uv_os_sock_t sock) {
  int r;
  r = close(sock);
  ASSERT(r == 0);
}

static connection_context_t *
create_connection_context(uv_os_sock_t sock, int is_server_connection) {
  int r;
  connection_context_t *context;

  context = (connection_context_t *)malloc(sizeof *context);
  ASSERT(context != NULL);

  context->sock = sock;
  context->is_server_connection = is_server_connection;
  context->read = 0;
  context->sent = 0;
  context->open_handles = 0;
  context->events = 0;
  context->delayed_events = 0;
  context->got_fin = 0;
  context->sent_fin = 0;
  context->got_disconnect = 0;

  r = uv_poll_init_socket(uv_default_loop(), &context->poll_handle, sock);
  context->open_handles++;
  context->poll_handle.data = context;
  ASSERT(r == 0);

  r = uv_timer_init(uv_default_loop(), &context->timer_handle);
  context->open_handles++;
  context->timer_handle.data = context;
  ASSERT(r == 0);

  return context;
}

static void connection_close_cb(uv_handle_t *handle) {
  connection_context_t *context = (connection_context_t *)handle->data;

  if (--context->open_handles == 0) {
    if (test_mode == DUPLEX || context->is_server_connection) {
      ASSERT(context->read == TRANSFER_BYTES);
    } else {
      ASSERT(context->read == 0);
    }

    if (test_mode == DUPLEX || !context->is_server_connection) {
      ASSERT(context->sent == TRANSFER_BYTES);
    } else {
      ASSERT(context->sent == 0);
    }

    closed_connections++;

    free(context);
  }
}

static void destroy_connection_context(connection_context_t *context) {
  uv_close((uv_handle_t *)&context->poll_handle, connection_close_cb);
  uv_close((uv_handle_t *)&context->timer_handle, connection_close_cb);
}

static void connection_poll_cb(uv_poll_t *handle, int status, int events) {
  connection_context_t *context = (connection_context_t *)handle->data;
  unsigned int new_events;
  int r;

  ASSERT(status == 0);
  ASSERT(events & context->events);
  ASSERT(!(events & ~context->events));

  new_events = context->events;

  if (events & UV_READABLE) {
    int action = rand() % 7;

    switch (action) {
    case 0:
    case 1: {
      /* Read a couple of bytes. */
      static char buffer[74];
      r = recv(context->sock, buffer, sizeof buffer, 0);
      ASSERT(r >= 0);

      if (r > 0) {
        context->read += r;
      } else {
        /* Got FIN. */
        context->got_fin = 1;
        new_events &= ~UV_READABLE;
      }

      break;
    }

    case 2:
    case 3: {
      /* Read until EAGAIN. */
      static char buffer[931];
      r = recv(context->sock, buffer, sizeof buffer, 0);
      ASSERT(r >= 0);

      while (r > 0) {
        context->read += r;
        r = recv(context->sock, buffer, sizeof buffer, 0);
      }

      if (r == 0) {
        /* Got FIN. */
        context->got_fin = 1;
        new_events &= ~UV_READABLE;
      } else {
        ASSERT(got_eagain());
      }

      break;
    }

    case 4:
      /* Ignore. */
      break;

    case 5:
      /* Stop reading for a while. Restart in timer callback. */
      new_events &= ~UV_READABLE;
      if (!uv_is_active((uv_handle_t *)&context->timer_handle)) {
        context->delayed_events = UV_READABLE;
        uv_timer_start(&context->timer_handle, delay_timer_cb, 10, 0);
      } else {
        context->delayed_events |= UV_READABLE;
      }
      break;

    case 6:
      /* Fudge with the event mask. */
      uv_poll_start(&context->poll_handle, UV_WRITABLE, connection_poll_cb);
      uv_poll_start(&context->poll_handle, UV_READABLE, connection_poll_cb);
      context->events = UV_READABLE;
      break;

    default:
      ASSERT(0);
    }
  }

  if (events & UV_WRITABLE) {
    if (context->sent < TRANSFER_BYTES &&
        !(test_mode == UNIDIRECTIONAL && context->is_server_connection)) {
      /* We have to send more bytes. */
      int action = rand() % 7;

      switch (action) {
      case 0:
      case 1: {
        /* Send a couple of bytes. */
        static char buffer[103];

        int send_bytes = MIN(TRANSFER_BYTES - context->sent, sizeof buffer);
        ASSERT(send_bytes > 0);

        r = send(context->sock, buffer, send_bytes, 0);

        if (r < 0) {
          ASSERT(got_eagain());
          spurious_writable_wakeups++;
          break;
        }

        ASSERT(r > 0);
        context->sent += r;
        valid_writable_wakeups++;
        break;
      }

      case 2:
      case 3: {
        /* Send until EAGAIN. */
        static char buffer[1234];

        int send_bytes = MIN(TRANSFER_BYTES - context->sent, sizeof buffer);
        ASSERT(send_bytes > 0);

        r = send(context->sock, buffer, send_bytes, 0);

        if (r < 0) {
          ASSERT(got_eagain());
          spurious_writable_wakeups++;
          break;
        }

        ASSERT(r > 0);
        valid_writable_wakeups++;
        context->sent += r;

        while (context->sent < TRANSFER_BYTES) {
          send_bytes = MIN(TRANSFER_BYTES - context->sent, sizeof buffer);
          ASSERT(send_bytes > 0);

          r = send(context->sock, buffer, send_bytes, 0);

          if (r <= 0)
            break;
          context->sent += r;
        }
        ASSERT(r > 0 || got_eagain());
        break;
      }

      case 4:
        /* Ignore. */
        break;

      case 5:
        /* Stop sending for a while. Restart in timer callback. */
        new_events &= ~UV_WRITABLE;
        if (!uv_is_active((uv_handle_t *)&context->timer_handle)) {
          context->delayed_events = UV_WRITABLE;
          uv_timer_start(&context->timer_handle, delay_timer_cb, 100, 0);
        } else {
          context->delayed_events |= UV_WRITABLE;
        }
        break;

      case 6:
        /* Fudge with the event mask. */
        uv_poll_start(&context->poll_handle, UV_READABLE, connection_poll_cb);
        uv_poll_start(&context->poll_handle, UV_WRITABLE, connection_poll_cb);
        context->events = UV_WRITABLE;
        break;

      default:
        ASSERT(0);
      }

    } else {
      /* Nothing more to write. Send FIN. */
      int r;
      r = shutdown(context->sock, SHUT_WR);
      ASSERT(r == 0);
      context->sent_fin = 1;
      new_events &= ~UV_WRITABLE;
    }
  }
  if (events & UV_DISCONNECT) {
    context->got_disconnect = 1;
    ++disconnects;
    new_events &= ~UV_DISCONNECT;
  }

  if (context->got_fin && context->sent_fin && context->got_disconnect) {
    /* Sent and received FIN. Close and destroy context. */
    close_socket(context->sock);
    destroy_connection_context(context);
    context->events = 0;

  } else if (new_events != context->events) {
    /* Poll mask changed. Call uv_poll_start again. */
    context->events = new_events;
    uv_poll_start(handle, new_events, connection_poll_cb);
  }

  /* Assert that uv_is_active works correctly for poll handles. */
  if (context->events != 0) {
    ASSERT(1 == uv_is_active((uv_handle_t *)handle));
  } else {
    ASSERT(0 == uv_is_active((uv_handle_t *)handle));
  }
}

static void delay_timer_cb(uv_timer_t *timer) {
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

static server_context_t *create_server_context(uv_os_sock_t sock) {
  int r;
  server_context_t *context;

  context = (server_context_t *)malloc(sizeof *context);
  ASSERT(context != NULL);

  context->sock = sock;
  context->connections = 0;

  r = uv_poll_init_socket(uv_default_loop(), &context->poll_handle, sock);
  context->poll_handle.data = context;
  ASSERT(r == 0);

  return context;
}

static void server_close_cb(uv_handle_t *handle) {
  server_context_t *context = (server_context_t *)handle->data;
  free(context);
}

static void destroy_server_context(server_context_t *context) {
  uv_close((uv_handle_t *)&context->poll_handle, server_close_cb);
}

static void server_poll_cb(uv_poll_t *handle, int status, int events) {
  UNUSEDPARAM events;
  UNUSEDPARAM status;

  server_context_t *server_context = (server_context_t *)handle->data;
  connection_context_t *connection_context;
  struct sockaddr_in addr;
  socklen_t addr_len;
  uv_os_sock_t sock;
  int r;

  addr_len = sizeof addr;
  sock = accept(server_context->sock, (struct sockaddr *)&addr, &addr_len);
  ASSERT(sock >= 0);

  connection_context = create_connection_context(sock, 1);
  connection_context->events = UV_READABLE | UV_WRITABLE | UV_DISCONNECT;
  r = uv_poll_start(&connection_context->poll_handle,
                    UV_READABLE | UV_WRITABLE | UV_DISCONNECT,
                    connection_poll_cb);
  ASSERT(r == 0);

  if (++server_context->connections == NUM_CLIENTS) {
    close_socket(server_context->sock);
    destroy_server_context(server_context);
  }
}

static void start_server(void) {
  server_context_t *context;
  struct sockaddr_in addr;
  uv_os_sock_t sock;
  int r;

  ASSERT(0 == uv_ip4_addr("127.0.0.1", TEST_PORT, &addr));
  sock = create_bound_socket(addr);
  context = create_server_context(sock);

  r = listen(sock, 100);
  ASSERT(r == 0);

  r = uv_poll_start(&context->poll_handle, UV_READABLE, server_poll_cb);
  ASSERT(r == 0);
}

static void start_client(void) {
  uv_os_sock_t sock;
  connection_context_t *context;
  struct sockaddr_in server_addr;
  struct sockaddr_in addr;
  int r;

  ASSERT(0 == uv_ip4_addr("127.0.0.1", TEST_PORT, &server_addr));
  ASSERT(0 == uv_ip4_addr("0.0.0.0", 0, &addr));

  sock = create_bound_socket(addr);
  context = create_connection_context(sock, 0);

  context->events = UV_READABLE | UV_WRITABLE | UV_DISCONNECT;
  r = uv_poll_start(&context->poll_handle,
                    UV_READABLE | UV_WRITABLE | UV_DISCONNECT,
                    connection_poll_cb);
  ASSERT(r == 0);

  r = connect(sock, (struct sockaddr *)&server_addr, sizeof server_addr);
  ASSERT(r == 0 || got_eagain());
}

static void start_poll_test(void) {
  int i, r;

  start_server();

  for (i = 0; i < NUM_CLIENTS; i++)
    start_client();

  r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  ASSERT(r == 0);

  /* Assert that at most five percent of the writable wakeups was spurious. */
  ASSERT(spurious_writable_wakeups == 0 ||
         (valid_writable_wakeups + spurious_writable_wakeups) /
                 spurious_writable_wakeups >
             20);

  ASSERT(closed_connections == NUM_CLIENTS * 2);
  ASSERT(disconnects == NUM_CLIENTS * 2);
  MAKE_VALGRIND_HAPPY();
}

int main(int argc, char *argv[]) {
  UNUSEDPARAM argc;
  UNUSEDPARAM argv;

  test_mode = DUPLEX;
  printf("Hello World!\n");

  start_poll_test();
  return 0;
}
