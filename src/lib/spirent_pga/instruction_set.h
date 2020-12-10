#ifndef _LIB_SPIRENT_PGA_INSTRUCTION_SET_H_
#define _LIB_SPIRENT_PGA_INSTRUCTION_SET_H_

#include <array>
#include <string_view>

namespace pga::instruction_set {

/* Scalar instructions are always available */
constexpr bool scalar_enabled = true;

/* Turn our nasty ifdef's into nice boolean constants */
#ifdef ISPC_TARGET_AUTOMATIC
constexpr bool automatic_enabled = true;
#else
constexpr bool automatic_enabled = false;
#endif

#ifdef ISPC_TARGET_SSE2
constexpr bool sse2_enabled = true;
#else
constexpr bool sse2_enabled = false;
#endif

#ifdef ISPC_TARGET_SSE4
constexpr bool sse4_enabled = true;
#else
constexpr bool sse4_enabled = false;
#endif

#ifdef ISPC_TARGET_AVX
constexpr bool avx_enabled = true;
#else
constexpr bool avx_enabled = false;
#endif

#ifdef ISPC_TARGET_AVX2
constexpr bool avx2_enabled = true;
#else
constexpr bool avx2_enabled = false;
#endif

#ifdef ISPC_TARGET_AVX512SKX
constexpr bool avx512skx_enabled = true;
#else
constexpr bool avx512skx_enabled = false;
#endif

#ifdef ISPC_TARGET_NEON
constexpr bool neon_enabled = true;
#else
constexpr bool neon_enabled = false;
#endif

enum class type {
    NONE = 0,
    SCALAR,
    AUTO,
    SSE2,
    SSE4,
    AVX,
    AVX2,
    AVX512SKX,
    NEON,
    MAX
};

template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

constexpr std::string_view to_string(type t)
{
    constexpr auto type_names = associative_array<type, std::string_view>(
        std::pair(type::SCALAR, "scalar"),
        std::pair(type::AUTO, "ISPC automatic"),
        std::pair(type::SSE2, "SSE2"),
        std::pair(type::SSE4, "SSE4"),
        std::pair(type::AVX, "AVX"),
        std::pair(type::AVX2, "AVX2"),
        std::pair(type::AVX512SKX, "AVX512"),
        std::pair(type::NEON, "NEON"));

    auto cursor = std::begin(type_names), end = std::end(type_names);
    while (cursor != end) {
        if (cursor->first == t) return (cursor->second);
        cursor++;
    }

    return ("unknown");
}

constexpr bool enabled(type t)
{
    constexpr auto sets_enabled = associative_array<type, bool>(
        std::pair(type::SCALAR, scalar_enabled),
        std::pair(type::AUTO, automatic_enabled),
        std::pair(type::SSE2, sse2_enabled),
        std::pair(type::SSE4, sse4_enabled),
        std::pair(type::AVX, avx_enabled),
        std::pair(type::AVX2, avx2_enabled),
        std::pair(type::AVX512SKX, avx512skx_enabled),
        std::pair(type::NEON, neon_enabled));

    auto cursor = std::begin(sets_enabled), end = std::end(sets_enabled);
    while (cursor != end) {
        if (cursor->first == t) return (cursor->second);
        cursor++;
    }
    return (false);
}

bool available(type t);

} // namespace pga::instruction_set

#endif /* _LIB_SPIRENT_PGA_INSTRUCTION_SET_H_ */
