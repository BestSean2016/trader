#include <gtest/gtest.h>
#include <iostream>
#include "utility.h"
#include "trader.h"
#include "mongo.h"

using namespace std;

extern "C" void test_connection_poll_cb(uv_poll_t *handle, int status, int events);

#define ipaddr "127.0.0.1"
#define testport 12345

connection_poll_cb conn_poll_cb = 0;

TEST(libuv_poll, test_server_client) {
    int i, r;
    NUM_CLIENTS = 10;

    conn_poll_cb = test_connection_poll_cb;

    start_server(ipaddr, testport);

    for (i = 0; i < NUM_CLIENTS; i++)
      start_client(ipaddr, testport);

    r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    EXPECT_EQ(r, 0);

    /* Assert that at most five percent of the writable wakeups was spurious. */
    EXPECT_EQ((spurious_writable_wakeups == 0 ||
           (valid_writable_wakeups + spurious_writable_wakeups) /
                   spurious_writable_wakeups >
               20), true);

    EXPECT_EQ(closed_connections, NUM_CLIENTS * 2);
    EXPECT_EQ(disconnects, NUM_CLIENTS * 2);
    MAKE_VALGRIND_HAPPY();
}


TEST(mongo, init_cleanup) {
    int ret = mongo_init("127.0.0.1", 27017, "test", "mycoll");
    EXPECT_EQ(ret, 0);

    mongo_clean();
}

TEST(mongo, thread_client) {
    int ret = mongo_init("127.0.0.1", 27017, "test", "mycoll");
    EXPECT_EQ(ret, 0);

    MONGO* mongo_links[10];
    for (int i = 0; i < 10; ++i) {
        mongo_links[i] = mongo_open();
        EXPECT_NE(mongo_links[i], (void*)0);
    }


    for (int i = 0; i < 10; ++i) {
        mongo_close(mongo_links[i]);
    }

    mongo_clean();
}
