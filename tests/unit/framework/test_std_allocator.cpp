#include <cstdlib>
#include <numeric>
#include <unordered_set>

#include "catch.hpp"

#include "memory/std_allocator.h"
#include "memory/allocator/free_list.h"

struct c_deleter {
    void operator()(void* ptr) {
        free(ptr);
    }
};

using raw_allocator = openperf::memory::std_allocator<uint8_t, openperf::memory::allocator::free_list>;

TEST_CASE("basic functionality for std_allocator", "[std_allocator]")
{
    static constexpr size_t pool_size = 1024 * 1024;

    SECTION("can create, ") {
        /* Create a know size of memory to play with */
        std::unique_ptr<void, c_deleter> raw_pool(malloc(pool_size));
        REQUIRE(raw_pool);

        SECTION("can allocate, ") {
            auto mempool = raw_allocator(reinterpret_cast<uintptr_t>(raw_pool.get()),
                                         pool_size);

            auto a1 = mempool.allocate(1024);
            REQUIRE(a1);

            auto a2 = mempool.allocate(1024);
            REQUIRE(a2);

            REQUIRE(a1 != a2);

            SECTION("can deallocate, ") {
                mempool.deallocate(a1, 1024);
            }

            SECTION("can allocate after deallocation, ") {
                auto a3 = mempool.allocate(1024);
                REQUIRE(a3);
                REQUIRE(a2 != a3);

                SECTION("can deallocate again, ") {
                    mempool.deallocate(a3, 1024);
                }
            }

            mempool.deallocate(a2, 1024);
        }

        SECTION("get exception when out of memory, ") {
            static constexpr size_t alloc_size = 64 * 1024;

            auto mempool = raw_allocator(reinterpret_cast<uintptr_t>(raw_pool.get()),
                                         pool_size);

            std::vector<uint8_t*> ptrs;
            for (size_t allocated = 0;
                 allocated < (pool_size - alloc_size);
                 allocated += alloc_size) {
                ptrs.push_back(mempool.allocate(alloc_size));
                REQUIRE(ptrs.back() != nullptr);
            }

            /* Should be out of memory */
            REQUIRE_THROWS_AS(mempool.allocate(alloc_size),
                              std::bad_alloc);

            /* Release all memory */
            for (auto p : ptrs) {
                mempool.deallocate(p, alloc_size);
            }
        }

        SECTION("can churn through memory, ") {
            static constexpr size_t alloc_size = 32 * 1024;
            static constexpr size_t churn_loops = 16;

            auto mempool = raw_allocator(reinterpret_cast<uintptr_t>(raw_pool.get()),
                                         pool_size);

            /**
             * Allocate a bunch of memory and release in random order.
             * Should verify ability to merge allocations back together.
             */
            for (size_t i = 0; i < churn_loops; i++) {
                std::unordered_set<uint8_t*> ptrs;
                for (size_t allocated = 0;
                     allocated < (pool_size - alloc_size);
                     allocated += alloc_size) {
                    auto p = mempool.allocate(alloc_size);
                    REQUIRE(p != nullptr);
                    ptrs.insert(p);
                }

                for (auto p : ptrs) {
                    mempool.deallocate(p, alloc_size);
                }
            }

            SECTION("memory has been merged together, ") {
                /*
                 * Allocate a huge chunk; only possible if memory is not
                 * fragmented.
                 */
                auto p = mempool.allocate(pool_size - alloc_size);
                REQUIRE(p != nullptr);
                mempool.deallocate(p, pool_size - alloc_size);
            }
        }
    }
}

using int_allocator = openperf::memory::std_allocator<int, openperf::memory::allocator::free_list>;

TEST_CASE("able to use std_allocator in a container", "[std_allocator]")
{
    static constexpr size_t pool_size = 1024 * 1024 * 10;
    std::unique_ptr<void, c_deleter> raw_pool(malloc(pool_size));
    REQUIRE(raw_pool);
    auto int_pool = int_allocator(reinterpret_cast<uintptr_t>(raw_pool.get()),
                                  pool_size);

    SECTION("can handle container allocations, ") {
        std::vector<int, int_allocator> test_vec(int_pool);
        for (int i = 1; i < 100; i++) {
            test_vec.push_back(i);
        }

        REQUIRE(std::accumulate(std::begin(test_vec), std::end(test_vec), 0)
                == (test_vec.back() * (test_vec.back() + 1)) / 2);

        SECTION("can free container allocations, ") {
            test_vec.clear();
            test_vec.shrink_to_fit();
        }
    }
}
