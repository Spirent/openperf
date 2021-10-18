#include <random>
#include <vector>
#include <algorithm>

#include "catch.hpp"
#include "cpu/generator/matrix_functions.hpp"
#include "test_ispc_target.hpp"

using isa_type = openperf::cpu::instruction_set::type;
using namespace openperf::cpu::generator;

TEST_CASE("CPU matrix multiplication", "[cpu]")
{
    SECTION("SCALAR")
    {
        REQUIRE(enabled(isa_type::SCALAR));
        REQUIRE(available(isa_type::SCALAR));

        // Matrix A:
        // {12, 52, 49, 20},
        // {94, 59, 19, 58},
        // {58, 24, 61, 37},
        // {15, 72, 48, 38},
        std::vector<int32_t> matrix_a = {
            12, 52, 49, 20, 94, 59, 19, 58, 58, 24, 61, 37, 15, 72, 48, 38};

        // Matrix B:
        // {91, 18, 40, 27},
        // {71, 15, 72, 37},
        // {49, 82, 27, 18},
        // {52, 81, 38, 10},
        std::vector<int32_t> matrix_b = {
            91, 18, 40, 27, 71, 15, 72, 37, 49, 82, 27, 18, 52, 81, 38, 10};

        // Matrix R = A x B:
        // {8225,  6634, 6307,  3330},
        // {16690, 8833, 10725, 5643},
        // {11895, 9403, 7101,  3922},
        // {10805, 8364, 8524,  4313},
        std::vector<int32_t> matrix_r = {8225,
                                         6634,
                                         6307,
                                         3330,
                                         16690,
                                         8833,
                                         10725,
                                         5643,
                                         11895,
                                         9403,
                                         7101,
                                         3922,
                                         10805,
                                         8364,
                                         8524,
                                         4313};

        std::vector<int32_t> matrix_c(16);
        scalar::multiply_matrix_int32(
            matrix_a.data(), matrix_b.data(), matrix_c.data(), 4);

        REQUIRE(std::equal(matrix_c.begin(), matrix_c.end(), matrix_r.begin()));
    }

    SECTION("SIMD")
    {
        std::random_device rnd_device;
        std::mt19937 rnd_gen(rnd_device());

        std::vector<int32_t> matrix_a, matrix_b, matrix_c, matrix_r;

        auto get_random = [&rnd_gen]() {
            static std::uniform_int_distribution<> rnd_distrib(1, 100);
            return rnd_distrib(rnd_gen);
        };

        constexpr auto matrix_size = 32;
        matrix_a.resize(matrix_size * matrix_size);
        matrix_b.resize(matrix_a.size());
        matrix_c.resize(matrix_a.size());
        matrix_r.resize(matrix_a.size());

        std::generate(matrix_a.begin(), matrix_a.end(), get_random);
        std::generate(matrix_b.begin(), matrix_b.end(), get_random);

        std::string benchmark_name = "Multiply matrix "
                                     + std::to_string(matrix_size) + "x"
                                     + std::to_string(matrix_size);

        BENCHMARK(benchmark_name + " SCALAR")
        {
            scalar::multiply_matrix_int32(
                matrix_a.data(), matrix_b.data(), matrix_r.data(), matrix_size);
        };

        auto& wrapper = matrix_functions::instance().multiply_matrix_int32;
        bool vector_test_was_executed = false;

        for (auto set : cpu::test::vector_instruction_sets()) {
            auto instruction = std::string(to_string(set));
            std::for_each(instruction.begin(), instruction.end(), [](auto& c) {
                c = ::toupper(c);
            });

            if (!enabled(set)) {
                WARN("Instruction set " << instruction << " not enabled");
                continue;
            }

            if (!available(set)) {
                WARN("Skipping "
                     << instruction
                     << " function tests due to lack of host CPU support.");
                continue;
            }

            if (auto mul = wrapper.function(set); mul != nullptr) {
                vector_test_was_executed = true;
                std::fill(matrix_c.begin(), matrix_c.end(), 0);

                BENCHMARK(benchmark_name + " " + instruction)
                {
                    mul(matrix_a.data(),
                        matrix_b.data(),
                        matrix_c.data(),
                        matrix_size);
                };

                REQUIRE(std::equal(
                    matrix_r.begin(), matrix_r.end(), matrix_c.begin()));
            } else {
                WARN("Instruction set " << instruction << " wasn't tested");
            }
        }

        REQUIRE(vector_test_was_executed);
    }
}
