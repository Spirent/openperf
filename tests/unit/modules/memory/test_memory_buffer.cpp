#include "catch.hpp"
#include "memory/buffer.hpp"

using openperf::memory::internal::buffer;

void fill_buffer(buffer& buf, std::function<uint8_t(size_t)> filler)
{
    auto pointer = reinterpret_cast<uint8_t*>(buf.data());
    for (size_t i = 0; i < buf.size(); i++) pointer[i] = filler(i);
}

void test_buffer(buffer& buf, std::function<void(uint8_t, size_t)> checker)
{
    auto pointer = reinterpret_cast<uint8_t*>(buf.data());
    for (size_t i = 0; i < buf.size(); i++) checker(pointer[i], i);
}

TEST_CASE("Memory Buffer", "[buffer]")
{
    size_t size = 1024;

    SECTION("allocating")
    {
        {
            auto buf = buffer();
            buf.allocate(size);

            auto ptr = reinterpret_cast<uint8_t*>(buf.data());
            for (size_t i = 0; i < size; i++) {
                ptr[i] = i % 0xFF;
                REQUIRE(ptr[i] == i % 0xFF);
            }

            buf.release();

            REQUIRE(buf.size() == 0);
            REQUIRE(buf.data() == nullptr);
        }
        {
            auto buf = buffer(16);
            buf.allocate(size);

            auto ptr = reinterpret_cast<uint8_t*>(buf.data());
            for (size_t i = 0; i < size; i++) {
                ptr[i] = i % 0xFF;
                REQUIRE(ptr[i] == i % 0xFF);
            }

            buf.release();

            REQUIRE(buf.size() == 0);
            REQUIRE(buf.data() == nullptr);
        }
    }

    SECTION("resizing")
    {
        auto buf = buffer(16);
        buf.allocate(size);
        REQUIRE(buf.size() == size);

        fill_buffer(buf, [](size_t i) { return i % 0xFF; });

        buf.resize(size * 2);
        REQUIRE(buf.size() == size * 2);

        auto pointer = reinterpret_cast<uint8_t*>(buf.data());
        for (size_t i = 0; i < size; i++) REQUIRE(pointer[i] == i % 0xFF);
    }

    SECTION("moving")
    {
        auto buffer0 = buffer();
        buffer0.allocate(size);

        fill_buffer(buffer0, [](size_t i) { return i % 0xFF; });

        auto buffer1 = buffer(std::move(buffer0));
        REQUIRE(buffer0.size() == 0);
        REQUIRE(buffer0.data() == nullptr);
        REQUIRE(buffer1.size() == size);
        REQUIRE(buffer1.data() != nullptr);

        test_buffer(buffer1,
                    [](uint8_t item, size_t i) { REQUIRE(item == i % 0xFF); });
    }

    SECTION("copying")
    {
        auto buffer0 = buffer();
        buffer0.allocate(size);

        fill_buffer(buffer0, [](size_t i) { return i % 0xFF; });

        auto buffer1 = buffer(buffer0);
        REQUIRE(buffer0.size() == buffer1.size());
        REQUIRE(buffer0.data() != buffer1.data());

        test_buffer(buffer1,
                    [](uint8_t item, size_t i) { REQUIRE(item == i % 0xFF); });
    }

    SECTION("assigning")
    {
        auto buffer0 = buffer();
        buffer0.allocate(size);

        fill_buffer(buffer0, [](size_t i) { return i % 0xFF; });

        auto buffer1 = buffer();
        REQUIRE(buffer0.size() != buffer1.size());

        buffer1 = buffer0;
        REQUIRE(buffer0.size() == buffer1.size());
        REQUIRE(buffer0.data() != buffer1.data());

        test_buffer(buffer1,
                    [](uint8_t item, size_t i) { REQUIRE(item == i % 0xFF); });
    }
}