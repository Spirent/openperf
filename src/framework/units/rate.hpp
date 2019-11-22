#ifndef _OP_UNITS_RATE_HPP_
#define _OP_UNITS_RATE_HPP_

/**
 * @file
 *
 * A standard template for handling rates that borrows heavily from the
 * implementation of std::chrono::duration.  A rate is essentially the same
 * as a duration but associates the Representation with a ratio specifying
 * the frequency instead of the period.
 *
 * In addition to standard operations and casts as found in
 * std::chrono::duration, we also provide a helper function to generate a
 * duration based on the frequency.
 */

#include <limits>
#include <numeric>
#include <ratio>

namespace openperf::units {

template <typename Rep, typename Frequency = std::ratio<1>> class rate;

/**
 * Utility structs
 **/

template <typename T> struct is_rate : std::false_type
{};

template <typename Rep, typename Frequency>
struct is_rate<rate<Rep, Frequency>> : std::true_type
{};

template <typename Rep, typename Frequency>
struct is_rate<const rate<Rep, Frequency>> : std::true_type
{};

template <typename T> struct is_ratio : std::false_type
{};

template <intmax_t Num, intmax_t Den>
struct is_ratio<std::ratio<Num, Den>> : std::true_type
{};

template <typename Rep> struct rate_values
{
    static constexpr Rep zero() { return Rep(0); }
    static constexpr Rep min() { return std::numeric_limits<Rep>::lowest(); }
    static constexpr Rep max() { return std::numeric_limits<Rep>::max(); }
};

template <typename Rep>
struct treat_as_floating_point : std::is_floating_point<Rep>
{};

/**
 * Casting functions
 **/

template <typename FromRate, typename ToRate,
          typename Frequency = typename std::ratio_divide<
              typename FromRate::frequency, typename ToRate::frequency>::type>
struct rate_cast_impl
{
    ToRate operator()(const FromRate& from) const
    {
        using common =
            std::common_type_t<typename ToRate::rep, typename FromRate::rep>;
        return ToRate(static_cast<typename ToRate::rep>(
            static_cast<common>(from.count())
            * static_cast<common>(Frequency::num)
            / static_cast<common>(Frequency::den)));
    }
};

template <typename ToRate, typename Rep, typename Frequency>
constexpr typename std::enable_if_t<is_rate<ToRate>::value, ToRate>
rate_cast(const rate<Rep, Frequency>& from)
{
    return (rate_cast_impl<rate<Rep, Frequency>, ToRate>()(from));
}

/*
 * Get the period of the rate as a (std::chrono)duration.
 * This is similar to the rate_cast above, but period is the inverse of
 * frequency, so the conversion is flipped.
 */
template <typename FromRate, typename ToDuration,
          typename Frequency = typename std::ratio_multiply<
              typename FromRate::frequency, typename ToDuration::period>::type>
struct get_period_impl
{
    ToDuration operator()(const FromRate& from) const
    {
        using common = std::common_type_t<typename ToDuration::rep,
                                          typename FromRate::rep>;
        return (ToDuration(static_cast<typename ToDuration::rep>(
            static_cast<common>(Frequency::den)
            / (static_cast<common>(from.count())
               * static_cast<common>(Frequency::num)))));
    }
};

template <typename ToDuration, typename Rep, typename Frequency>
constexpr ToDuration get_period(const rate<Rep, Frequency>& from)
{
    return (get_period_impl<rate<Rep, Frequency>, ToDuration>()(from));
}

/**
 * Rate representation wrapper
 **/

template <typename Rep, typename Frequency> class rate
{
    static_assert(!is_rate<Rep>::value, "Rate representation cannot be a rate");
    static_assert(is_ratio<Frequency>::value,
                  "Rate frequency must be a std::ratio");
    static_assert(Frequency::num > 0, "Frequency must be positive");

    Rep rep_;

public:
    using rep = Rep;
    using frequency = Frequency;

    constexpr rate() = default;

    template <typename Rep2, typename = typename std::enable_if_t<
                                 std::is_convertible<const Rep2&, rep>::value
                                 && (treat_as_floating_point<rep>::value
                                     || !treat_as_floating_point<Rep2>::value)>>
    constexpr explicit rate(const Rep2& r)
        : rep_(static_cast<rep>(r))
    {}

    template <typename Rep2, typename Frequency2,
              typename = typename std::enable_if_t<
                  treat_as_floating_point<rep>::value
                  || (std::ratio_divide<Frequency2, frequency>::den == 1
                      && !treat_as_floating_point<Rep2>::value)>>
    constexpr rate(const rate<Rep2, Frequency2>& r)
        : rep_(rate_cast<rate>(r).count())
    {}

    ~rate() = default;
    rate(const rate&) = default;
    rate& operator=(const rate& other) = default;

    constexpr Rep count() const { return (rep_); }

    constexpr std::common_type_t<rate> operator+() const
    {
        return (std::common_type_t<rate>(*this));
    }

    constexpr std::common_type_t<rate> operator-() const
    {
        return (std::common_type_t<rate>(-rep_));
    }

    constexpr rate& operator+=(const rate& rhs)
    {
        rep_ += rhs.count();
        return (*this);
    }

    constexpr rate& operator-=(const rate& rhs)
    {
        rep_ -= rhs.count();
        return (*this);
    }

    constexpr rate& operator*=(const Rep& rhs)
    {
        rep_ *= rhs;
        return (*this);
    }

    constexpr rate& operator/=(const Rep& rhs)
    {
        rep_ /= rhs;
        return (*this);
    }

    constexpr rate& operator%=(const Rep& rhs)
    {
        rep_ %= rhs;
        return (*this);
    }

    constexpr rate& operator%=(const rate& rhs)
    {
        rep_ %= rhs.count();
        return (*this);
    }

    static constexpr rate zero() noexcept
    {
        return (rate(rate_values<Rep>::zero()));
    }

    static constexpr rate min() noexcept
    {
        return (rate(rate_values<Rep>::min()));
    }

    static constexpr rate max() noexcept
    {
        return (rate(rate_values<Rep>::max()));
    }
};

/**
 * Comparison operators
 **/

/* Equality */
template <typename LeftRate, typename RightRate> struct rate_equal_impl
{
    bool operator()(const LeftRate& left, const RightRate& right) const
    {
        using common = std::common_type_t<LeftRate, RightRate>;
        return (common(left).count() == common(right).count());
    }
};

template <typename Rate> struct rate_equal_impl<Rate, Rate>
{
    bool operator()(const Rate& left, const Rate& right) const
    {
        return (left.count() == right.count());
    }
};

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr bool operator==(const rate<Rep1, Frequency1>& left,
                          const rate<Rep2, Frequency2>& right)
{
    return (rate_equal_impl<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>()(
        left, right));
}

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr bool operator!=(const rate<Rep1, Frequency1>& left,
                          const rate<Rep2, Frequency2>& right)
{
    return (!rate_equal_impl<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>()(
        left, right));
}

/* Less than */
template <typename LeftRate, typename RightRate> struct rate_less_than_impl
{
    bool operator()(const LeftRate& left, const RightRate& right) const
    {
        using common = std::common_type_t<LeftRate, RightRate>;
        return (common(left).count() < common(right).count());
    }
};

template <typename Rate> struct rate_less_than_impl<Rate, Rate>
{
    bool operator()(const Rate& left, const Rate& right) const
    {
        return (left.count() < right.count());
    }
};

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr bool operator<(const rate<Rep1, Frequency1>& left,
                         const rate<Rep2, Frequency2>& right)
{
    return (
        rate_less_than_impl<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>()(
            left, right));
}

/* Everything else (implemented with less than) */
template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr bool operator>(const rate<Rep1, Frequency1>& left,
                         const rate<Rep2, Frequency2>& right)
{
    return (right < left);
}

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr bool operator<=(const rate<Rep1, Frequency1>& left,
                          const rate<Rep2, Frequency2>& right)
{
    return !(right < left);
}

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr bool operator>=(const rate<Rep1, Frequency1>& left,
                          const rate<Rep2, Frequency2>& right)
{
    return !(left < right);
}

/**
 * Arithmetic operators
 **/

/* addition */
template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr
    typename std::common_type_t<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>
    operator+(const rate<Rep1, Frequency1>& left,
              const rate<Rep2, Frequency2>& right)
{
    using common =
        std::common_type_t<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>;
    return (common(common(left).count() + common(right).count()));
}

/* subtraction */
template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr
    typename std::common_type_t<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>
    operator-(const rate<Rep1, Frequency1>& left,
              const rate<Rep2, Frequency2>& right)
{
    using common =
        std::common_type_t<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>;
    return (common(common(left).count() - common(right).count()));
}

/* multiplication */
template <typename Rep1, typename Frequency, typename Rep2>
constexpr typename std::enable_if_t<
    std::is_convertible<Rep2, std::common_type_t<Rep1, Rep2>>::value,
    rate<typename std::common_type_t<Rep1, Rep2>, Frequency>>
operator*(const rate<Rep1, Frequency>& value, const Rep2& scalar)
{
    using common_rep = std::common_type_t<Rep1, Rep2>;
    using common_rate = rate<common_rep, Frequency>;
    return (common_rate(common_rate(value).count()
                        * static_cast<common_rep>(scalar)));
}

template <typename Rep1, typename Frequency, typename Rep2>
constexpr typename std::enable_if_t<
    std::is_convertible<Rep2, std::common_type_t<Rep1, Rep2>>::value,
    rate<typename std::common_type_t<Rep1, Rep2>, Frequency>>
operator*(const Rep1& scalar, const rate<Rep2, Frequency>& value)
{
    return (value * scalar);
}

/* division */
template <typename Rep1, typename Frequency, typename Rep2>
constexpr typename std::enable_if_t<
    std::is_convertible<Rep2, std::common_type_t<Rep1, Rep2>>::value,
    rate<typename std::common_type_t<Rep1, Rep2>, Frequency>>
operator/(const rate<Rep1, Frequency>& value, const Rep2& scalar)
{
    using common_rep = std::common_type_t<Rep1, Rep2>;
    using common_rate = rate<common_rep, Frequency>;
    return (common_rate(common_rate(value).count()
                        / static_cast<common_rep>(scalar)));
}

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr typename std::common_type_t<Rep1, Rep2>
operator/(const rate<Rep1, Frequency1>& left, rate<Rep2, Frequency2>& right)
{
    using common =
        std::common_type_t<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>;
    return (common(left).count() / common(right).count());
}

/* modulo */
template <typename Rep1, typename Frequency, typename Rep2>
constexpr typename std::enable_if_t<
    std::is_convertible<Rep2, std::common_type_t<Rep1, Rep2>>::value,
    rate<typename std::common_type_t<Rep1, Rep2>, Frequency>>
operator%(const rate<Rep1, Frequency>& value, const Rep2& scalar)
{
    using common_rep = std::common_type_t<Rep1, Rep2>;
    using common_rate = rate<common_rep, Frequency>;
    return (common_rate(common_rate(value).count()
                        % static_cast<common_rep>(scalar)));
}

template <typename Rep1, typename Frequency1, typename Rep2,
          typename Frequency2>
constexpr typename std::common_type_t<Rep1, Rep2>
operator%(const rate<Rep1, Frequency1>& left, rate<Rep2, Frequency2>& right)
{
    using common =
        std::common_type_t<rate<Rep1, Frequency1>, rate<Rep2, Frequency2>>;
    return (common(left).count() % common(right).count());
}

} // namespace openperf::units

namespace std {

template <class Rep1, class Frequency1, class Rep2, class Frequency2>
struct common_type<openperf::units::rate<Rep1, Frequency1>,
                   openperf::units::rate<Rep2, Frequency2>>
{
    typedef openperf::units::rate<
        typename common_type<Rep1, Rep2>::type,
        typename std::ratio<std::gcd(Frequency1::num, Frequency2::num),
                            std::lcm(Frequency1::den, Frequency2::den)>>
        type;
};

} // namespace std

#endif /* _OP_UNITS_RATE_HPP_ */
