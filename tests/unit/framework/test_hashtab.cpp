#include <thread>
#include <vector>

#include "catch.hpp"

#include "core/icp_core.h"

TEST_CASE("icp_hashtab functionality checks", "[hashtab]")
{
    SECTION("can create, ") {
        auto tab = icp_hashtab_allocate();
        REQUIRE(tab);

        SECTION("can insert, ") {
            uintptr_t key = 0xdeadbeef;
            uintptr_t value = 0xc0ffee;
            bool inserted = icp_hashtab_insert(tab,
                                               reinterpret_cast<void*>(key),
                                               reinterpret_cast<void*>(value));
            REQUIRE(inserted);

            SECTION("can retrieve, ") {
                uintptr_t retrieve = reinterpret_cast<uintptr_t>(
                    icp_hashtab_find(tab, reinterpret_cast<void*>(key)));
                REQUIRE(retrieve == 0xc0ffee);
            }

            SECTION("can count, ") {
                REQUIRE(icp_hashtab_size(tab) == 1);
            }

            SECTION("can delete, ") {
                bool deleted = icp_hashtab_delete(tab, reinterpret_cast<void*>(key));
                REQUIRE(deleted);

                SECTION("cannot find deleted, ") {
                    auto not_here = icp_hashtab_find(tab, reinterpret_cast<void*>(key));
                    REQUIRE(not_here == nullptr);
                }
            }
        }

        SECTION("can free, ") {
            icp_hashtab_free(&tab);
            REQUIRE(tab == nullptr);
        }
    }
}

struct icp_hashtab_deleter {
    void operator()(icp_hashtab *tab) const {
        icp_hashtab_free(&tab);
    }
};

TEST_CASE("icp_hashtab concurrent inserts work", "[hashtab]")
{
    /*
     * Use a bunch of large prime numbers for scalers.  This allows us
     * to insert a bunch of interspersed values without duplicates.
     */
    std::vector<uintptr_t> scalers = { 1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049 };
    std::vector<std::thread> threads;
    static constexpr uintptr_t test_value = 0x5ca1ab1e;
    static constexpr size_t nb_inserts = 250;

    SECTION("can create, ") {
        std::unique_ptr<icp_hashtab, icp_hashtab_deleter> tab(icp_hashtab_allocate());
        REQUIRE(tab);

        SECTION("can insert with threads, ") {
            for (auto scaler : scalers) {
                threads.emplace_back(std::thread([&, scaler]() {
                                                     for (size_t i = 1; i <= nb_inserts; i++) {
                                                         uintptr_t key = scaler * i;
                                                         bool inserted = icp_hashtab_insert(tab.get(),
                                                                                            reinterpret_cast<void*>(key),
                                                                                            reinterpret_cast<void*>(test_value));
                                                         REQUIRE(inserted);
                                                     }
                                                 }));
            }

            for (auto& thread : threads) {
                thread.join();
            }

            SECTION("can accurately count inserted items, ") {
                REQUIRE(icp_hashtab_size(tab.get()) == nb_inserts * scalers.size());
            }

            SECTION("can find all inserted items, ") {
                for (auto scaler : scalers) {
                    for (size_t i = 1; i < nb_inserts; i++) {
                        uintptr_t key = scaler * i;
                        auto found = icp_hashtab_find(tab.get(),
                                                      reinterpret_cast<void*>(key));
                        REQUIRE(reinterpret_cast<uintptr_t>(found) == test_value);
                    }
                }
            }
        }
    }
}
