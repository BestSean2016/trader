#include "utility.h"
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


static test_mode_t test_mode = DUPLEX;

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
