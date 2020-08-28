
#include <random>
#include <vector>
#include <algorithm>
#include <iostream>

#include "catch.hpp"
#include "cpu/matrix.hpp"
#include "cpu/instruction_set.hpp"
#include "cpu/function_wrapper.hpp"

using openperf::cpu::instruction_set;
using namespace openperf::cpu::internal;

template <typename T> void matrix_print(const T* m, int size)
{
    for (int i = 0; i < size * size; i++) {
        std::cout << m[i] << "\t";
        if ((i + 1) % size == 0) std::cout << std::endl;
    }
    std::cout << std::endl;
}

using function_t = void (*)(uint32_t*, uint32_t*, uint32_t*, uint32_t);
auto wrapper = function_wrapper<function_t>();

TEST_CASE("CPU matrix multiplication", "[cpu]")
{
    SECTION("SCALAR")
    {
        // Matrix A:
        // {12, 52, 49, 20},
        // {94, 59, 19, 58},
        // {58, 24, 61, 37},
        // {15, 72, 48, 38},
        std::vector<uint32_t> matrix_a = {
            12, 52, 49, 20, 94, 59, 19, 58, 58, 24, 61, 37, 15, 72, 48, 38};

        // Matrix B:
        // {91, 18, 40, 27},
        // {71, 15, 72, 37},
        // {49, 82, 27, 18},
        // {52, 81, 38, 10},
        std::vector<uint32_t> matrix_b = {
            91, 18, 40, 27, 71, 15, 72, 37, 49, 82, 27, 18, 52, 81, 38, 10};

        // Matrix R = A x B:
        // {8225,  6634, 6307,  3330},
        // {16690, 8833, 10725, 5643},
        // {11895, 9403, 7101,  3922},
        // {10805, 8364, 8524,  4313},
        std::vector<uint32_t> matrix_r = {8225,
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

        std::vector<uint32_t> matrix_c(16);
        scalar::multiplicate_matrix_uint32(
            matrix_a.data(), matrix_b.data(), matrix_c.data(), 4);

        // matrix_print(matrix_c.data(), 4);
        // matrix_print(matrix_r.data(), 4);
        REQUIRE(std::equal(matrix_c.begin(), matrix_c.end(), matrix_r.begin()));
    }

    SECTION("SIMD")
    {
        std::random_device rnd_device;
        std::mt19937 rnd_gen(rnd_device());

        std::vector<uint32_t> matrix_a, matrix_b, matrix_c, matrix_r;

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
        scalar::multiplicate_matrix_uint32(
            matrix_a.data(), matrix_b.data(), matrix_r.data(), matrix_size);

        auto instruction_list = {instruction_set::SCALAR,
                                 instruction_set::SSE2,
                                 instruction_set::SSE4,
                                 instruction_set::AVX,
                                 instruction_set::AVX2,
                                 instruction_set::AVX512};

        for (auto set : instruction_list) {
            if (!enabled(set)) {
                WARN("Instruction set " << to_string(set) << " not enabled");
                continue;
            }

            if (!available(set)) {
                WARN("Skipping "
                     << to_string(set)
                     << " function tests due to lack of host CPU support.");
                continue;
            }

            if (auto mul = wrapper.function(set); mul != nullptr) {
                std::fill(matrix_c.begin(), matrix_c.end(), 0);

                std::string benchmark_name = "Multiplicate matrix "
                                             + std::to_string(matrix_size) + "x"
                                             + std::to_string(matrix_size) + " "
                                             + std::string(to_string(set));

                BENCHMARK(benchmark_name)
                {
                    return mul(matrix_a.data(),
                               matrix_b.data(),
                               matrix_c.data(),
                               matrix_size);
                }

                REQUIRE(std::equal(
                    matrix_c.begin(), matrix_c.end(), matrix_r.begin()));
            } else {
                WARN("Instruction set " << to_string(set) << " wasn't tested");
            }
        }
    }
}
