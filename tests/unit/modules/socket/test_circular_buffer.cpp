#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include "catch.hpp"

#include "socket/circular_buffer_consumer.tcc"
#include "socket/circular_buffer_producer.tcc"

struct circular_buffer
{
    /*
     * Unfortunately, smart pointers are too smart for their own good here.
     * We need to touch the pointer in only one constructor.
     */
    uint8_t* base_ptr;
    size_t base_len;
    std::atomic_size_t base_read_idx;
    std::atomic_size_t base_write_idx;

    circular_buffer(size_t len)
        : base_ptr(new uint8_t[len])
        , base_len(len)
        , base_read_idx(0)
        , base_write_idx(0)
    {}
    circular_buffer() {}
    ~circular_buffer()
    {
        delete[] base_ptr;
    }
};

class test_buffer_producer : public circular_buffer
                           , public openperf::socket::circular_buffer_producer<test_buffer_producer>
{
    friend class circular_buffer_producer<test_buffer_producer>;

protected:
    uint8_t* producer_base() const { return base_ptr; }
    size_t producer_len() const { return base_len; }
    std::atomic_size_t& producer_read_idx() { return base_read_idx; }
    const std::atomic_size_t& producer_read_idx() const { return base_read_idx; }
    std::atomic_size_t& producer_write_idx() { return base_write_idx; }
    const std::atomic_size_t& producer_write_idx() const { return base_write_idx; }

public:
    test_buffer_producer(size_t capacity)
        : circular_buffer(capacity)
    {}
    ~test_buffer_producer() = default;
};

class test_buffer_consumer : public circular_buffer
                           , public openperf::socket::circular_buffer_consumer<test_buffer_consumer>
{
    friend class circular_buffer_consumer<test_buffer_consumer>;

protected:
    uint8_t* consumer_base() const { return base_ptr; }
    size_t consumer_len() const { return base_len; }
    std::atomic_size_t& consumer_read_idx() { return base_read_idx; }
    const std::atomic_size_t& consumer_read_idx() const { return base_read_idx; }
    std::atomic_size_t& consumer_write_idx() { return base_write_idx; }
    const std::atomic_size_t& consumer_write_idx() const { return base_write_idx; }

public:
    test_buffer_consumer()
        : circular_buffer()
    {}
    ~test_buffer_consumer() = default;
};

TEST_CASE("circular buffer functionality", "[circular buffer]")
{
    static constexpr size_t test_buffer_size = 4096;
    auto producer = std::make_unique<test_buffer_producer>(test_buffer_size);
    /*
     * Yeah this is a little weird.  The consumer and producer are different view
     * of the same underlying structure.
     */
    auto consumer = new (producer.get()) test_buffer_consumer();

    SECTION("can query buffer state, ") {
        REQUIRE(producer->writable() == test_buffer_size);
        REQUIRE(consumer->readable() == 0);

        SECTION("can write data, ") {
            static constexpr std::string_view test_string = "This is a silly test string.";
            auto written = producer->write(reinterpret_cast<const uint8_t*>(test_string.data()),
                                           test_string.length());
            REQUIRE(written == test_string.length());
            REQUIRE(consumer->readable() == test_string.length());

            SECTION("can read written data, ") {
                std::array<uint8_t, 1024> buffer;
                auto read = consumer->read(buffer.data(), buffer.size());
                REQUIRE(read == test_string.length());
                REQUIRE(consumer->readable() == 0);
                REQUIRE(std::memcmp(buffer.data(), test_string.data(), read) == 0);
            }

            SECTION("can read written data a byte at a time, ") {
                std::array<uint8_t, 1> char_buffer;
                size_t count = 0;
                while (consumer->readable()) {
                    auto read = consumer->read(char_buffer.data(), char_buffer.size());
                    REQUIRE(read == 1);
                    count++;
                }
                REQUIRE(count == test_string.length());
            }
        }

        SECTION("can fill buffer with data, ") {
            std::array<uint8_t, 2048> buffer;
            buffer.fill('a');

            size_t written = 0;
            while (producer->writable() > 0) {
                written += producer->write(buffer.data(),
                                           std::min(producer->writable(), buffer.size()));
            }

            REQUIRE(producer->writable() == 0);
            REQUIRE(consumer->readable() == written);
        }
    }

    SECTION("can wrap around, ") {
        static constexpr size_t vec_size = test_buffer_size / 2;
        for (auto value : {1, 2, 3, 4, 5, 6, 7, 8}) {
            std::vector<uint8_t> data(vec_size, value);
            std::array<uint8_t, vec_size>  buffer;
            /* read and write the data into the buffer */
            REQUIRE(producer->writable() >= vec_size);
            auto written = producer->write(data.data(), vec_size);
            REQUIRE(written == vec_size);
            REQUIRE(consumer->readable() == vec_size);
            auto read = consumer->read(buffer.data(), vec_size);
            REQUIRE(read == vec_size);
            REQUIRE(consumer->readable() == 0);

            REQUIRE(std::memcmp(data.data(), buffer.data(), vec_size) == 0);
        }
    }

    SECTION("0 byte write/read is idempotent, ") {
        std::array<uint8_t, 128> buffer;
        buffer.fill('b');

        auto written = producer->write(buffer.data(), buffer.size());
        REQUIRE(written == buffer.size());
        REQUIRE(consumer->readable() == written);
        auto zero = producer->write(buffer.data(), 0);
        REQUIRE(zero == 0);
        REQUIRE(consumer->readable() == written);
        auto zero2 = consumer->read(buffer.data(), 0);
        REQUIRE(zero2 == 0);
        REQUIRE(consumer->readable() == written);
    }
}
