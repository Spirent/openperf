#ifndef _OP_UTILS_ENUM_FLAGS_H_
#define _OP_UTILS_ENUM_FLAGS_H_

#include <cstdio>
#include <type_traits>

/*
 * Type safe flag implementation for c++ enums.
 * Based on the concepts described here:
 * https://dalzhim.github.io/2017/08/11/Improving-the-enum-class-bitmask/
 */

#define declare_enum_flags(Enum)                                               \
    template <> struct openperf::utils::is_enum_flag<Enum> : std::true_type    \
    {}

namespace openperf::utils {

template <typename Enum> struct is_enum_flag : std::false_type
{};

/*
 * We have two strongly-typed values we'll deal with: enumerators and flags.
 * Enumerators represent explicitly named constants, e.g. the enum values.
 * Flags represent combinations of enumerators.
 */

template <typename Enum> struct enumerator
{
    using enum_type = std::underlying_type_t<Enum>;
    Enum value;

    constexpr enumerator(const Enum& value)
        : value(value)
    {}

    constexpr enumerator(const enum_type& value)
        : value(static_cast<Enum>(value))
    {}

    constexpr explicit operator bool() const
    {
        return (static_cast<enum_type>(value) != enum_type{0});
    }
};

template <typename Enum> struct bit_flags
{
    using enum_type = std::underlying_type_t<Enum>;
    enum_type value;

    constexpr bit_flags(const Enum& value)
        : value(static_cast<enum_type>(value))
    {}

    constexpr bit_flags(const enum_type& value)
        : value(value)
    {}

    constexpr explicit operator bool() const { return (value != enum_type{0}); }
};

} // namespace openperf::utils

/*
 * Here be operator overloads galore.
 * Since we have binary operators with multiple types, we end up with a lot
 * of possible combinations.  SFINAE is used to prevent regular enums values
 * from matching any of these functions.  Users must explicitly enable
 * flag semantics via the macro above.
 *
 * Note: These functions are intentionally in the global namespace for name
 * resolution purposes. The compiler was unable to find some functions when
 * they were defined in the openperf::utils namespace.
 */

/***
 * & operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const Enum& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(static_cast<enum_type>(lhs)
                                              & static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const openperf::utils::enumerator<Enum>& lhs,
          const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(
        static_cast<enum_type>(lhs.value) & static_cast<enum_type>(rhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator&(const openperf::utils::bit_flags<Enum>& lhs,
          const openperf::utils::bit_flags<Enum>& rhs)
{
    return (openperf::utils::bit_flags<Enum>(lhs.value & rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const Enum& lhs, const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(
        static_cast<enum_type>(lhs) & static_cast<enum_type>(rhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const openperf::utils::enumerator<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(static_cast<enum_type>(lhs.value)
                                              & static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const Enum& lhs, const openperf::utils::bit_flags<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(static_cast<enum_type>(lhs)
                                              & rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const openperf::utils::bit_flags<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(lhs.value
                                              & static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const openperf::utils::enumerator<Enum>& lhs,
          const openperf::utils::bit_flags<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(static_cast<enum_type>(lhs.value)
                                              & rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::enumerator<Enum>>
operator&(const openperf::utils::bit_flags<Enum>& lhs,
          const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::enumerator<Enum>(
        lhs.value & static_cast<enum_type>(rhs.value)));
}

/***
 * | operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const Enum& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs)
                                             | static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const openperf::utils::enumerator<Enum>& lhs,
          const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(
        static_cast<enum_type>(lhs.value) | static_cast<enum_type>(rhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const openperf::utils::bit_flags<Enum>& lhs,
          const openperf::utils::bit_flags<Enum>& rhs)
{
    return (openperf::utils::bit_flags<Enum>(lhs.value | rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const Enum& lhs, const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(
        static_cast<enum_type>(lhs) | static_cast<enum_type>(rhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const openperf::utils::enumerator<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs.value)
                                             | static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const Enum& lhs, const openperf::utils::bit_flags<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs)
                                             | rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const openperf::utils::bit_flags<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(lhs.value
                                             | static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const openperf::utils::enumerator<Enum>& lhs,
          const openperf::utils::bit_flags<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs.value)
                                             | rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|(const openperf::utils::bit_flags<Enum>& lhs,
          const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(
        lhs.value | static_cast<enum_type>(rhs.value)));
}

/***
 * ^ operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const Enum& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs)
                                             ^ static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const openperf::utils::enumerator<Enum>& lhs,
          const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(
        static_cast<enum_type>(lhs.value) ^ static_cast<enum_type>(rhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const openperf::utils::bit_flags<Enum>& lhs,
          const openperf::utils::bit_flags<Enum>& rhs)
{
    return (openperf::utils::bit_flags<Enum>(lhs.value ^ rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const Enum& lhs, const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(
        static_cast<enum_type>(lhs) ^ static_cast<enum_type>(rhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const openperf::utils::enumerator<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs.value)
                                             ^ static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const Enum& lhs, const openperf::utils::bit_flags<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs)
                                             ^ rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const openperf::utils::bit_flags<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(lhs.value
                                             ^ static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const openperf::utils::enumerator<Enum>& lhs,
          const openperf::utils::bit_flags<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(static_cast<enum_type>(lhs.value)
                                             ^ rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^(const openperf::utils::bit_flags<Enum>& lhs,
          const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(
        lhs.value ^ static_cast<enum_type>(rhs.value)));
}

/***
 * &= operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator&=(openperf::utils::bit_flags<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    lhs.value &= static_cast<enum_type>(rhs);
    return (lhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator&=(openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    lhs.value &= static_cast<enum_type>(rhs.value);
    return (lhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator&=(openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::bit_flags<Enum>& rhs)
{
    lhs.value &= rhs.value;
    return (lhs);
}

/***
 * |= operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|=(openperf::utils::bit_flags<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    lhs.value |= static_cast<enum_type>(rhs);
    return (lhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|=(openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    lhs.value |= static_cast<enum_type>(rhs.value);
    return (lhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator|=(openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::bit_flags<Enum>& rhs)
{
    lhs.value |= rhs.value;
    return (lhs);
}

/***
 * ^= operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^=(openperf::utils::bit_flags<Enum>& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    lhs.value ^= static_cast<enum_type>(rhs);
    return (lhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^=(openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::enumerator<Enum>& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    lhs.value ^= static_cast<enum_type>(rhs.value);
    return (lhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator^=(openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::bit_flags<Enum>& rhs)
{
    lhs.value ^= rhs.value;
    return (lhs);
}

/***
 * ~ operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator~(const Enum& lhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (openperf::utils::bit_flags<Enum>(~static_cast<enum_type>(lhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator~(const openperf::utils::enumerator<Enum>& lhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (
        openperf::utils::bit_flags<Enum>(~static_cast<enum_type>(lhs.value)));
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    openperf::utils::bit_flags<Enum>>
operator~(const openperf::utils::bit_flags<Enum>& lhs)
{
    return (openperf::utils::bit_flags<Enum>(~lhs.value));
}

/***
 * == operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator==(const Enum& lhs, const Enum& rhs)
{
    return (lhs == rhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator==(const openperf::utils::enumerator<Enum>& lhs,
           const openperf::utils::enumerator<Enum>& rhs)
{
    return (lhs.value == rhs.value);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator==(const openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::bit_flags<Enum>& rhs)
{
    return (lhs.value == rhs.value);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator==(const Enum& lhs, const openperf::utils::enumerator<Enum>& rhs)
{
    return (lhs == rhs.value);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator==(const openperf::utils::enumerator<Enum>& lhs, const Enum& rhs)
{
    return (lhs.value == rhs);
}

/***
 * != operators
 ***/

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator!=(const Enum& lhs, const Enum& rhs)
{
    return (lhs != rhs);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator!=(const openperf::utils::enumerator<Enum>& lhs,
           const openperf::utils::enumerator<Enum>& rhs)
{
    return (lhs.value != rhs.value);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator!=(const openperf::utils::bit_flags<Enum>& lhs,
           const openperf::utils::bit_flags<Enum>& rhs)
{
    return (lhs.value != rhs.value);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator!=(const Enum& lhs, const openperf::utils::enumerator<Enum>& rhs)
{
    return (lhs != rhs.value);
}

template <typename Enum>
constexpr typename std::enable_if_t<
    std::is_enum_v<Enum> && openperf::utils::is_enum_flag<Enum>::value,
    bool>
operator!=(const openperf::utils::enumerator<Enum>& lhs, const Enum& rhs)
{
    return (lhs.value != rhs);
}

#endif /* _OP_UTILS_ENUM_FLAGS_H_ */
