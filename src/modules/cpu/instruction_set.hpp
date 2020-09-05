#ifndef _CPU_INSTRUCTION_SET_HPP_
#define _CPU_INSTRUCTION_SET_HPP_

#include <array>
#include <string_view>
#include <utility>

#include "ispc/ispc.hpp"

namespace openperf::cpu {

enum class instruction_set : uint8_t {
    NONE = 0,
    SCALAR,
    SSE2,
    SSE4,
    AVX,
    AVX2,
    AVX512,
    MAX
};

template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

constexpr std::string_view to_string(instruction_set t)
{
    constexpr auto instruction_set_names =
        associative_array<instruction_set, std::string_view>(
            std::pair(instruction_set::SCALAR, "scalar"),
            std::pair(instruction_set::SSE2, "sse2"),
            std::pair(instruction_set::SSE4, "sse4"),
            std::pair(instruction_set::AVX, "avx"),
            std::pair(instruction_set::AVX2, "avx2"),
            std::pair(instruction_set::AVX512, "avx512"));

    auto cursor = std::begin(instruction_set_names),
         end = std::end(instruction_set_names);
    while (cursor != end) {
        if (cursor->first == t) return (cursor->second);
        cursor++;
    }

    return ("unknown");
}

constexpr cpu::instruction_set to_instruction_set(std::string_view value)
{
    if (value == "scalar") return cpu::instruction_set::SCALAR;
    if (value == "sse2") return cpu::instruction_set::SSE2;
    if (value == "sse4") return cpu::instruction_set::SSE4;
    if (value == "avx") return cpu::instruction_set::AVX;
    if (value == "avx2") return cpu::instruction_set::AVX2;
    if (value == "avx512") return cpu::instruction_set::AVX512;

    throw std::runtime_error("Error from string to instruction_set converting: "
                             "Illegal string value");
}

constexpr bool enabled(instruction_set t)
{
    constexpr auto sets_enabled = associative_array<instruction_set, bool>(
        std::pair(instruction_set::SCALAR, true),
        std::pair(instruction_set::SSE2, ispc::sse2_enabled),
        std::pair(instruction_set::SSE4, ispc::sse4_enabled),
        std::pair(instruction_set::AVX, ispc::avx_enabled),
        std::pair(instruction_set::AVX2, ispc::avx2_enabled),
        std::pair(instruction_set::AVX512, ispc::avx512skx_enabled));

    auto cursor = std::begin(sets_enabled), end = std::end(sets_enabled);
    while (cursor != end) {
        if (cursor->first == t) return (cursor->second);
        cursor++;
    }
    return (false);
}

bool available(instruction_set t);

} // namespace openperf::cpu

#endif /* _CPU_INSTRUCTION_SET_HPP_ */
