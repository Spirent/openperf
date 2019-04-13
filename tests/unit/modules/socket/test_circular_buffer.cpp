#include <array>
#include <cstring>
#include <string>

#include "catch.hpp"

#include "socket/circular_buffer.h"

using circular_buffer = icp::socket::circular_buffer;

TEST_CASE("basic functionality for circular_buffer", "[circular_buffer]")
{
    SECTION("can create, ") {
        static constexpr size_t test_size = 10 * 1024;
        auto cb = circular_buffer::server(test_size);
        REQUIRE(cb.writable() >= test_size);
        REQUIRE(cb.readable() == 0);
        REQUIRE(cb.empty() == true);

        SECTION("can write data, ") {
            static constexpr std::string_view test_string = "This is a silly test string.";
            auto result = cb.write(reinterpret_cast<const uint8_t*>(test_string.data()),
                                   test_string.length());
            REQUIRE(result == test_string.length());
            REQUIRE(cb.readable() == test_string.length());
            REQUIRE(cb.readable() + cb.writable() == cb.capacity());

            SECTION("can read written data, ") {
                std::array<uint8_t, 1024> buffer;
                auto read = cb.read(buffer.data(), buffer.size());
                REQUIRE(read == test_string.length());
                REQUIRE(cb.readable() == 0);
                REQUIRE(memcmp(buffer.data(), test_string.data(), read) == 0);
            }

            SECTION("can read subset of data, ") {
                std::array<uint8_t, 1> char_buffer;
                size_t count = 0;
                while (cb.readable()) {
                    auto read = cb.read(char_buffer.data(), 1);
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
            while (cb.writable() > 0) {
                written += cb.write(buffer.data(), std::min(cb.writable(), buffer.size()));
                REQUIRE(cb.readable() + cb.writable() == cb.capacity());
            }

            REQUIRE(cb.full());
            REQUIRE(cb.writable() == 0);
            REQUIRE(cb.readable() == written);
        }
    }

    SECTION("can wrap around, ") {
        /*
         * A priori knowledge that buffer is a multiple of 4k required.
         */
        static constexpr size_t test_size = 4096;
        auto cb = circular_buffer::server(test_size);
        REQUIRE(cb.writable() == test_size);

        static constexpr size_t vec_size = test_size / 2 + 1;
        for (auto value : {1, 2, 3, 4, 5, 6, 7, 8}) {
            std::vector<uint8_t> data(vec_size, value);
            std::array<uint8_t, vec_size>  buffer;
            /* read and write the data into the buffer */
            REQUIRE(cb.writable() >= vec_size);
            auto result = cb.write(data.data(), vec_size);
            REQUIRE(result == vec_size);
            REQUIRE(cb.readable() == vec_size);
            REQUIRE(cb.readable() + cb.writable() == cb.capacity());
            auto read = cb.read(buffer.data(), vec_size);
            REQUIRE(read == vec_size);
            REQUIRE(cb.readable() == 0);

            REQUIRE(memcmp(data.data(), buffer.data(), vec_size) == 0);
        }
    }
}
