#include "utility.h"
#include "trader.h"

static void delay_timer_cb(uv_timer_t *timer) {
  connection_context_t *context = (connection_context_t *)timer->data;
  int r;

  /* Timer should auto stop. */
  ASSERT(0 == uv_is_active((uv_handle_t *)timer));

  /* Add the requested events to the poll mask. */
  ASSERT(context->delayed_events != 0);
  context->events |= context->delayed_events;
  context->delayed_events = 0;

  r = uv_poll_start(&context->poll_handle, context->events, conn_poll_cb);
  ASSERT(r == 0);
}



void test_connection_poll_cb(uv_poll_t *handle, int status, int events) {
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
      uv_poll_start(&context->poll_handle, UV_WRITABLE, test_connection_poll_cb);
      uv_poll_start(&context->poll_handle, UV_READABLE, test_connection_poll_cb);
      context->events = UV_READABLE;
      break;

    default:
      ASSERT(0);
    }
  }

  if (events & UV_WRITABLE) {
    if (context->sent < TRANSFER_BYTES &&
        !(context->is_server_connection)) {
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
        uv_poll_start(&context->poll_handle, UV_READABLE, test_connection_poll_cb);
        uv_poll_start(&context->poll_handle, UV_WRITABLE, test_connection_poll_cb);
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
    uv_poll_start(handle, new_events, test_connection_poll_cb);
  }

  /* Assert that uv_is_active works correctly for poll handles. */
  if (context->events != 0) {
    ASSERT(1 == uv_is_active((uv_handle_t *)handle));
  } else {
    ASSERT(0 == uv_is_active((uv_handle_t *)handle));
  }
}
