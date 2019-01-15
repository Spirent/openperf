#include <numeric>
#include <thread>
#include <vector>
#include <unistd.h>

#include "catch.hpp"

#include "socket/bipartite_ring.h"

TEST_CASE("basic functionality for bipartite_ring", "[bipartite_ring]")
{
    static constexpr size_t test_ring_size = 4;
    using test_ring = icp::socket::bipartite_ring<int, test_ring_size>;
    auto ring = test_ring::server();

    SECTION("can enqueue, ") {
        static int test_value = 1;
        ring.enqueue(test_value);
        REQUIRE(ring.size() == 1);
        REQUIRE(ring.full() == false);
        REQUIRE(ring.empty() == false);
        REQUIRE(ring.capacity() == (test_ring_size - 1));
        REQUIRE(ring.available() == 1);  /* item is available */
        REQUIRE(ring.ready() == 0);      /* but not ready to be dequeued */

        SECTION("can use enqueued value, ") {
            auto next_value = ring.unpack();
            REQUIRE(next_value);
            REQUIRE(*next_value == test_value);

            /* no more values should be available */
            REQUIRE(ring.available() == 0);

            SECTION("can repack after use, ") {
                ring.repack();
                REQUIRE(ring.ready() == 1);

                SECTION("can dequeue after repack, ") {
                    auto dequeue_value = ring.dequeue();
                    REQUIRE(dequeue_value);
                    REQUIRE(*dequeue_value == test_value);
                    /* verify all meta data */
                    REQUIRE(ring.size() == 0);
                    REQUIRE(ring.full() == false);
                    REQUIRE(ring.empty() == true);
                    REQUIRE(ring.capacity() == test_ring_size);
                    REQUIRE(ring.available() == 0);
                    REQUIRE(ring.ready() == 0);
                }
            }
        }
    }

    SECTION("can fill ring, ") {
        std::vector<int> data(test_ring_size);
        std::iota(begin(data), end(data), 0);

        REQUIRE(ring.empty());  /* sanity check */
        for (auto value : data) {
            REQUIRE(ring.enqueue(value) == true);
        }

        REQUIRE(ring.size() == test_ring_size);
        REQUIRE(ring.full() == true);
        REQUIRE(ring.empty() == false);
        REQUIRE(ring.capacity() == 0);
        REQUIRE(ring.available() == test_ring_size);
        REQUIRE(ring.ready() == 0);

        SECTION("can not overfill ring, ") {
            REQUIRE(ring.enqueue(-1) == false);

            /* verify meta data has not changed */
            REQUIRE(ring.size() == test_ring_size);
            REQUIRE(ring.full() == true);
            REQUIRE(ring.empty() == false);
            REQUIRE(ring.capacity() == 0);
            REQUIRE(ring.available() == test_ring_size);
            REQUIRE(ring.ready() == 0);
        }

        SECTION("can use all available, ") {
            for (size_t idx = 0; idx < test_ring_size; idx++) {
                REQUIRE(ring.available() > 0);
                auto value = ring.unpack();
                REQUIRE(value);
                REQUIRE(*value == data[idx]);
            }

            /* verify meta data */
            REQUIRE(ring.size() == test_ring_size);
            REQUIRE(ring.full() == true);
            REQUIRE(ring.empty() == false);
            REQUIRE(ring.capacity() == 0);
            REQUIRE(ring.available() == 0);
            REQUIRE(ring.ready() == 0);

            SECTION("repack updates ready counter, ") {
                ring.repack();
                REQUIRE(ring.available() == 0);
                REQUIRE(ring.ready() == test_ring_size);

                SECTION("can dequeue all ready values, ") {
                    for (size_t idx = 0; idx < test_ring_size; idx++) {
                        REQUIRE(ring.ready() > 0);
                        auto value = ring.dequeue();
                        REQUIRE(value);
                        REQUIRE(*value == data[idx]);
                    }

                    /* verify meta data (again) */
                    REQUIRE(ring.size() == 0);
                    REQUIRE(ring.full() == false);
                    REQUIRE(ring.empty() == true);
                    REQUIRE(ring.capacity() == test_ring_size);
                    REQUIRE(ring.available() == 0);
                    REQUIRE(ring.ready() == 0);

                    SECTION("can not dequeue non-existant value, ") {
                        auto value = ring.dequeue();
                        REQUIRE(!value);
                    }
                }
            }
        }
    }

    SECTION("can bulk enqueue, ") {
        std::vector<int> data(test_ring_size);
        std::iota(begin(data), end(data), 0);

        REQUIRE(ring.empty());  /* sanity check */
        REQUIRE(ring.capacity() == test_ring_size);
        auto enqueued = ring.enqueue(data.data(), test_ring_size);
        REQUIRE(enqueued == test_ring_size);

        REQUIRE(ring.size() == test_ring_size);
        REQUIRE(ring.full() == true);
        REQUIRE(ring.empty() == false);
        REQUIRE(ring.capacity() == 0);
        REQUIRE(ring.available() == test_ring_size);
        REQUIRE(ring.ready() == 0);

        SECTION("can bulk unpack, ") {
            std::array<int*, test_ring_size> unpacked_values;
            auto unpacked = ring.unpack(unpacked_values.data(), test_ring_size);
            REQUIRE(unpacked == test_ring_size);

            /* verify meta data */
            REQUIRE(ring.size() == test_ring_size);
            REQUIRE(ring.full() == true);
            REQUIRE(ring.empty() == false);
            REQUIRE(ring.capacity() == 0);
            REQUIRE(ring.available() == 0);
            REQUIRE(ring.ready() == 0);

            SECTION("repack updates ready counter, ") {
                ring.repack();
                REQUIRE(ring.available() == 0);
                REQUIRE(ring.ready() == test_ring_size);

                SECTION("can bulk dequeue, ") {
                    std::array<int, test_ring_size * 2> dequeue_values;
                    auto dequeued = ring.dequeue(dequeue_values.data(), test_ring_size * 2);
                    REQUIRE(dequeued == test_ring_size);

                    /* verify data */
                    for (size_t idx = 0; idx < test_ring_size; idx++) {
                        REQUIRE(dequeue_values[idx] == data[idx]);
                    }

                    /* verify meta data (again) */
                    REQUIRE(ring.size() == 0);
                    REQUIRE(ring.full() == false);
                    REQUIRE(ring.empty() == true);
                    REQUIRE(ring.capacity() == test_ring_size);
                    REQUIRE(ring.available() == 0);
                    REQUIRE(ring.ready() == 0);
                }
            }
        }
    }
}

TEST_CASE("can enqueue/dequeue and use in parallel", "[bipartite_ring]")
{
    static constexpr size_t test_data_size = 256;
    std::vector<int> test_data(test_data_size);
    std::iota(begin(test_data), end(test_data), 0);

    static constexpr size_t test_ring_size = 16;
    auto ring = icp::socket::bipartite_ring<int, test_ring_size>::server();

    /* enqueue/wait/dequeue values */
    auto server = std::thread([&]() {
                                  size_t enqueue_idx = 0;
                                  size_t dequeue_idx = 0;
                                  bool blocked = false;

                                  while (dequeue_idx < test_data_size) {
                                      while (!ring.full()) {
                                          ring.enqueue(test_data[enqueue_idx++]);
                                      }

                                      while(ring.ready() == 0) {
                                          blocked = true;
                                          usleep(10);
                                      }

                                      while(ring.ready()) {
                                          auto value = ring.dequeue();
                                          REQUIRE(*value == test_data[dequeue_idx++]);
                                      }
                                  }

                                  REQUIRE(blocked);
                              });

    /* unpack and repack values */
    auto client = std::thread([&](){
                                  size_t unpack_idx = 0;
                                  bool blocked = false;

                                  while (unpack_idx < test_data_size) {
                                      while (ring.available() == 0) {
                                          blocked = true;
                                          usleep(10);
                                      }

                                      while(ring.available()) {
                                          auto value = ring.unpack();
                                          REQUIRE(value);
                                          REQUIRE(*value == test_data[unpack_idx++]);
                                      }

                                      ring.repack();
                                  }

                                  REQUIRE(blocked);
                              });

    server.join();
    client.join();
}
