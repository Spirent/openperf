#include <atomic>
#include <cstring>
#include <thread>
#include <unistd.h>

#include "catch.hpp"

#include "socket/event_queue_consumer.tcc"
#include "socket/event_queue_producer.tcc"

struct event_queue
{
    int base_fd;
    std::atomic_uint64_t base_read_idx;
    std::atomic_uint64_t base_write_idx;

    event_queue(int flags)
        : base_fd(eventfd(0, flags))
        , base_read_idx(0)
        , base_write_idx(0)
    {
        if (base_fd == -1) {
            throw std::runtime_error("Could not create eventfd: "
                                     + std::string(strerror(errno)));
        }
    }
    event_queue() {};
    ~event_queue() {
        close(base_fd);
    }
};

class test_event_producer : public event_queue
                          , public openperf::socket::event_queue_producer<test_event_producer>
{
    friend class event_queue_producer<test_event_producer>;

protected:
    int producer_fd() const { return base_fd; }
    std::atomic_uint64_t& notify_read_idx() { return base_read_idx; }
    const std::atomic_uint64_t& notify_read_idx() const { return base_read_idx; }
    std::atomic_uint64_t& notify_write_idx() { return base_write_idx; }
    const std::atomic_uint64_t& notify_write_idx() const { return base_write_idx; }

public:
    test_event_producer(int flags)
        : event_queue(flags)
    {}
    ~test_event_producer() = default;
};

class test_event_consumer : public event_queue
                          , public openperf::socket::event_queue_consumer<test_event_consumer>
{
    friend class event_queue_consumer<test_event_consumer>;

protected:
    int consumer_fd() const { return base_fd; }
    std::atomic_uint64_t& ack_read_idx() { return base_read_idx; }
    const std::atomic_uint64_t& ack_read_idx() const { return base_read_idx; }
    std::atomic_uint64_t& ack_write_idx() { return base_write_idx; }
    const std::atomic_uint64_t& ack_write_idx() const { return base_write_idx; }

public:
    test_event_consumer()
        : event_queue()
    {}
    ~test_event_consumer() = default;
};


TEST_CASE("event queue functionality", "[event queue]")
{
    SECTION("nonblocking testcases, ") {
        auto producer = std::make_unique<test_event_producer>(EFD_NONBLOCK);
        auto consumer = new (producer.get()) test_event_consumer();

        REQUIRE(producer);
        REQUIRE(consumer);

        SECTION("empty queue returns 0 on ack, ") {
            REQUIRE(consumer->ack() == 0);
        }

        SECTION("empty queue returns EWOULDBLOCK on ack_wait, ") {
            REQUIRE(consumer->ack_wait() == EWOULDBLOCK);
        }

        SECTION("notification behavior, ") {
            REQUIRE(producer->notify() == 0);

            SECTION("ack_wait returns 0 after notification, ") {
                REQUIRE(consumer->ack_wait() == 0);

                SECTION("2nd ack wait returns EWOULDBLOCK after notification, ") {
                    REQUIRE(consumer->ack_wait() == EWOULDBLOCK);
                }
            }
        }
    }

    SECTION("blocking testcases, ") {
        auto producer = std::make_unique<test_event_producer>(0);
        auto consumer = new (producer.get()) test_event_consumer();

        REQUIRE(producer);
        REQUIRE(consumer);

        SECTION("can wake sleeping thread with notify, ") {
            std::atomic_bool notified = false;
            bool notify_called = false;

            auto sleeper = std::thread([&]() {
                                           consumer->ack_wait();
                                           notified = true;
                                       });

            while (!notified) {
                producer->notify();
                notify_called = true;
            }
            REQUIRE(notify_called);
            sleeper.join();
        }

        SECTION("can wake sleeping thread with unblock, ") {
            std::atomic_bool unblocked = false;
            bool unblock_called = false;
            auto sleeper = std::thread([&]() {
                                           consumer->block_wait();
                                           unblocked = true;
                                       });

            while (!unblocked) {
                producer->unblock();
                unblock_called = true;
            }
            REQUIRE(unblock_called);
            sleeper.join();
        }
    }
}

