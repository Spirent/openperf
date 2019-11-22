#include "catch.hpp"

#include "core/op_init_factory.hpp"

struct test_base : openperf::core::init_factory<test_base, int&>
{
    test_base(Key){};
};

struct test_a : public test_base::registrar<test_a>
{
    test_a(int& counter) { counter++; }
};

struct test_b : public test_base::registrar<test_b>
{
    test_b(int& counter) { counter++; }
};

struct test_c : public test_base::registrar<test_c>
{
    test_c(int& counter) { counter++; }
};

TEST_CASE("verify init factory behavior", "[init_factory]")
{
    std::vector<std::unique_ptr<test_base>> test_objects;
    int counter = 0;
    test_base::make_all(test_objects, counter);
    REQUIRE(test_objects.size() == counter);
}
