#include <cstring>
#include <random>
#include <thread>
#include <vector>

#include <zmq.h>
#include "catch.hpp"

#include "core/op_core.h"

static std::string random_endpoint()
{
    static const std::string_view chars =
        "abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::mt19937_64 generator{std::random_device()()};
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    std::string to_return("inproc://test_task_sync_");
    std::generate_n(std::back_inserter(to_return), 8,
                    [&]{ return chars[dist(generator)]; });

    return (to_return);
}

static void ping_ponger(void *context, const char* endpoint,
                        const char* initial_syncpoint, int nb_syncs) {

    void* control = op_socket_get_client_subscription(context, endpoint, "");
    REQUIRE(control);

    op_task_sync_ping(context, initial_syncpoint);

    zmq_msg_t msg __attribute__((cleanup(zmq_msg_close)));

    REQUIRE(zmq_msg_init(&msg) == 0);

    for (int i = 0; i < nb_syncs; i++) {
        /* Retrieve an endpoint to sync to */
        REQUIRE(zmq_msg_recv(&msg, control, 0) > 0);
        std::string syncpoint(static_cast<char*>(zmq_msg_data(&msg)));

        /* And send sync reply */
        op_task_sync_ping(context, syncpoint.c_str());
    }

    zmq_close(control);
}

TEST_CASE("openperf task functionality checks", "[task]")
{
    void* context = zmq_ctx_new();
    REQUIRE(context);

    constexpr int nb_threads = 4;

    SECTION("single task sync") {
        std::vector<std::thread> threads;
        auto endpoint = random_endpoint();
        void *sync = op_task_sync_socket(context, endpoint.c_str());
        REQUIRE(sync);

        for (int i = 0; i < nb_threads; i++) {
            threads.emplace_back(std::thread([&]() {
                                                 op_task_sync_ping(context, endpoint.c_str());
                                             }));
        }

        REQUIRE(op_task_sync_block(&sync, nb_threads) == 0);

        for (auto& thread : threads) {
            thread.join();
        }
    }

    SECTION("repeated task syncs") {
        constexpr int nb_syncs = 100;
        std::vector<std::thread> threads;

        auto endpoint = random_endpoint();
        REQUIRE(endpoint.length());

        auto control = op_socket_get_server(context, ZMQ_PUB, endpoint.c_str());
        REQUIRE(control);

        auto initial_syncpoint = random_endpoint();
        REQUIRE(initial_syncpoint.length());
        auto init_sync = op_task_sync_socket(context, initial_syncpoint.c_str());
        REQUIRE(init_sync);

        for (int i = 0; i < nb_threads; i++) {
            threads.emplace_back(std::thread([&]() {
                                                 ping_ponger(context, endpoint.c_str(),
                                                             initial_syncpoint.c_str(), nb_syncs);
                                             }));
        }

        REQUIRE(op_task_sync_block(&init_sync, nb_threads) == 0);

        for (int k = 0; k < nb_syncs; k++) {
            /* Generate a new syncpoint... */
            auto syncpoint = random_endpoint();
            REQUIRE(syncpoint.length());

            auto sync = op_task_sync_socket(context, syncpoint.c_str());
            REQUIRE(sync);

            /* ... send it to our threads ... */
            zmq_msg_t to_send __attribute__((cleanup(zmq_msg_close)));
            REQUIRE(zmq_msg_init_size(&to_send, syncpoint.length() + 1) == 0);
            std::strcpy(reinterpret_cast<char*>(zmq_msg_data(&to_send)), syncpoint.c_str());
            REQUIRE(zmq_msg_send(&to_send, control, 0) > 0);

            /* ... and wait for their responses. */
            REQUIRE(op_task_sync_block(&sync, nb_threads) == 0);
        }

        for (auto& thread : threads) {
            thread.join();
        }

        zmq_close(control);
    }

    zmq_ctx_term(context);
}
