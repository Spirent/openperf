#include <memory>
#include <thread>
#include <vector>

#include "catch.hpp"

#include "core/op_core.h"
#include "core/op_list.hpp"

struct op_list_deleter
{
    void operator()(op_list* list) const { op_list_free(&list); }
};

static size_t _destroyer_run_count = 0;
static void* _destroyer_last_thing = NULL;

void test_thing_destroyer(void* thing)
{
    _destroyer_last_thing = thing;
    _destroyer_run_count++;
}

TEST_CASE("op_list functionality checks", "[list]")
{
    std::unique_ptr<op_list, op_list_deleter> list(op_list_allocate());

    REQUIRE(list);

    SECTION("items can be inserted and then found")
    {
        for (size_t i = 1; i < 10; i++) {
            op_list_item* item = op_list_item_allocate((void*)i);
            REQUIRE(item);
            REQUIRE(op_list_insert(list.get(), item) == true);
            REQUIRE(op_list_find(list.get(), (void*)i) != nullptr);
        }

        REQUIRE(op_list_find(list.get(), (void*)11) == nullptr);
    }

    SECTION("items can be inserted and then deleted")
    {
        for (size_t i = 1; i < 10; i++) {
            op_list_item* item = op_list_item_allocate((void*)i);
            REQUIRE(item);
            REQUIRE(op_list_insert(list.get(), item) == true);
            REQUIRE(op_list_delete(list.get(), (void*)i) == true);
            REQUIRE(op_list_find(list.get(), (void*)i) == nullptr);
        }
    }

    SECTION("verify garbage collection works")
    {
        op_list_set_destructor(list.get(), test_thing_destroyer);

        for (size_t i = 1; i < 10; i++) {
            op_list_item* item = op_list_item_allocate((void*)i);
            REQUIRE(item);
            REQUIRE(op_list_insert(list.get(), item) == true);
        }

        void* test_value = op_list_find(list.get(), (void*)5);
        REQUIRE(test_value);
        REQUIRE(op_list_delete(list.get(), (void*)5) == true);
        REQUIRE(op_list_find(list.get(), (void*)9) != nullptr);
        op_list_garbage_collect(list.get());

        /* Verify that our thing destroyer was called on test_value */
        REQUIRE(_destroyer_run_count == 1);
        REQUIRE(_destroyer_last_thing == test_value);
    }

    SECTION("verify that we can fully walk the list")
    {
        for (size_t i = 1; i < 10; i++) {
            op_list_item* item = op_list_item_allocate((void*)i);
            REQUIRE(item);
            REQUIRE(op_list_insert(list.get(), item) == true);
        }

        /* Now walk the list.  There should be 9 items */
        size_t nb_items = 0;
        void* value = nullptr;
        op_list_item* prev = op_list_head(list.get());
        REQUIRE(prev);
        while ((value = op_list_next(list.get(), &prev)) != nullptr) {
            nb_items++;
        }
        REQUIRE(nb_items == 9);
        REQUIRE(prev == nullptr);
    }

    SECTION("verify that concurrent insertion works")
    {
        /*
         * Use a bunch of large prime numbers for scalers.  This allows us
         * to insert a bunch of interspersed values without duplicates.
         */
        std::vector<unsigned> scalers = {1009, 1013, 1019, 1021,
                                         1031, 1033, 1039, 1049};
        std::vector<std::thread> threads;
        static constexpr size_t nb_inserts = 250;

        for (auto scaler : scalers) {
            threads.emplace_back(std::thread([&list, scaler]() {
                for (size_t i = 1; i <= nb_inserts; i++) {
                    uint64_t value = scaler * i;
                    op_list_item* item = op_list_item_allocate((void*)value);
                    REQUIRE(op_list_insert(list.get(), item) == true);
                }
            }));
        }

        for (auto& thread : threads) { thread.join(); }

        /* Verify that we have the correct number of items in our list */
        REQUIRE(op_list_length(list.get()) == nb_inserts * scalers.size());

        /* And verify that we can find every entry */
        for (auto scaler : scalers) {
            for (size_t i = 1; i <= nb_inserts; i++) {
                REQUIRE(op_list_find(list.get(), (void*)(scaler * i))
                        != nullptr);
            }
        }
    }

    SECTION("verify list snapshot")
    {
        static constexpr size_t test_items = 10;
        for (size_t i = 1; i <= test_items; i++) {
            REQUIRE(op_list_insert(list.get(), (void*)i) == true);
        }

        size_t** items = NULL;
        size_t nb_items = 0;

        REQUIRE(op_list_snapshot(list.get(), (void***)&items, &nb_items) == 0);
        REQUIRE(nb_items == test_items);
        REQUIRE(items != nullptr);

        free(items);
    }

    SECTION("verify list purge function")
    {
        for (size_t i = 1; i < 10; i++) {
            REQUIRE(op_list_insert(list.get(), (void*)i) == true);
        }

        REQUIRE(op_list_purge(list.get()) == 0);
        REQUIRE(op_list_length(list.get()) == 0); /* empty after purge */
    }
}

TEST_CASE("openperf list cpp wrapper functionality")
{
    SECTION("can create, ")
    {
        auto list = openperf::list<int>();

        SECTION("can insert, ")
        {
            int test_value = 4814;
            bool inserted = list.insert(&test_value);
            REQUIRE(inserted);

            SECTION("can count, ") { REQUIRE(list.size() == 1); }

            SECTION("can iterate, ")
            {
                size_t count = 0;
                for (auto item : list) {
                    REQUIRE(*item == test_value);
                    count++;
                }
                REQUIRE(count == list.size());
            }

            SECTION("can delete, ")
            {
                bool removed = list.remove(&test_value);
                REQUIRE(removed);

                REQUIRE(list.size() == 0);

                SECTION("can iterate empty list, ")
                {
                    for (auto item : list) {
                        (void)item;
                        REQUIRE(false);
                    }
                }
            }
        }
    }
}
