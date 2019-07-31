#include "catch.hpp"

#include "packetio/recycle.tcc"

template class icp::packetio::recycle::depot<2>;

using recycler = icp::packetio::recycle::depot<2>;

TEST_CASE("recycler functionality", "[recycler]")
{
    auto depot = recycler();

    SECTION("test absent reader, ") {
        depot.writer_add_reader(0);

        bool gc_fired = false;
        depot.writer_add_gc_callback([&](){ gc_fired = true; });

        depot.writer_process_gc_callbacks();

        /* GC runs because reader has never checked in */
        REQUIRE(gc_fired == true);
    }

    SECTION("test guarded reader, ") {
        depot.writer_add_reader(0);

        bool gc_fired = false;
        {
            icp::packetio::recycle::guard guard(depot, 0);
            depot.writer_add_gc_callback([&](){ gc_fired = true; });
            depot.writer_process_gc_callbacks();

            /* GC can't run because reader is guarded */
            REQUIRE(gc_fired == false);
        }

        /* GC can run when guard is destroyed */
        depot.writer_process_gc_callbacks();
        REQUIRE(gc_fired == true);
    }

    SECTION("test multiple readers, ") {
        depot.writer_add_reader(0);
        depot.writer_add_reader(1);

        bool gc_one_fired = false;
        bool gc_two_fired = false;

        /* Bump both readers to writer version 1 */
        depot.reader_checkpoint(0);
        depot.reader_checkpoint(1);

        /* send writer to version 2 */
        depot.writer_add_gc_callback([&](){ gc_one_fired = true; });

        /* Verify callback can't fire with both readers at version 1 */
        depot.writer_process_gc_callbacks();
        REQUIRE(gc_one_fired == false);

        /* Bump a reader to version 2 */
        depot.reader_checkpoint(0);

        /* Verify that callback still can't fire */
        depot.writer_process_gc_callbacks();
        REQUIRE(gc_one_fired == false);

        /* Add a new callback and make sure it can't fire either */
        depot.writer_add_gc_callback([&](){ gc_two_fired = true; });

        depot.writer_process_gc_callbacks();
        REQUIRE(gc_one_fired == false);
        REQUIRE(gc_two_fired == false);

        /* Bump both readers to the current writer version */
        depot.reader_checkpoint(0);
        depot.reader_checkpoint(1);

        /* Verify that both callbacks run */
        depot.writer_process_gc_callbacks();
        REQUIRE(gc_one_fired == true);
        REQUIRE(gc_two_fired == true);
    }
}
