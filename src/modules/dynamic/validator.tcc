#include "dynamic/api.hpp"

#include "swagger/v1/model/DynamicResultsConfig.h"

namespace openperf::dynamic {

namespace detail {
template <typename T> struct config_name
{};

template <> struct config_name<swagger::v1::model::ThresholdConfig>
{
    static constexpr auto value = "Threshold";
};

template <> struct config_name<swagger::v1::model::TDigestConfig>
{
    static constexpr auto value = "T-Digest";
};

template <typename T> std::string name()
{
    return (std::string(config_name<T>::value));
}

template <typename T, typename = std::void_t<>>
struct has_condition : std::false_type
{};

template <typename T>
struct has_condition<T, std::void_t<decltype(&T::getCondition)>>
    : std::true_type
{};

template <typename T>
inline constexpr auto has_condition_v = has_condition<T>::value;
} // namespace detail

template <typename Derived, typename Config>
static void is_valid(const Config& config, std::vector<std::string>& errors)
{
    auto stat_x = config.getStatX();

    if (stat_x.empty()) {
        errors.emplace_back(detail::name<Config>()
                            + " X statistic is required.");
    } else if (!Derived::is_valid_stat(normalize(stat_x))) {
        errors.emplace_back(detail::name<Config>() + " X statistic, " + stat_x
                            + ", is invalid.");
    }

    auto func = to_argument_function(config.getFunction());
    if (func == argument_t::NONE) {
        errors.emplace_back("Function, " + config.getFunction()
                            + ", is invalid.");
    }

    if constexpr (detail::has_condition_v<Config>) {
        auto comp = to_comparator(config.getCondition());
        if (comp == comparator::NONE) {
            errors.emplace_back("Condition, " + config.getCondition()
                                + ", is invalid.");
        }
    }

    if (func == argument_t::DXDY) {
        auto stat_y = config.getStatY();
        if (stat_y.empty()) {
            errors.emplace_back(detail::name<Config>()
                                + " Y statistic is required.");
        } else if (!Derived::is_valid_stat(normalize(stat_y))) {
            errors.emplace_back(detail::name<Config>() + " Y statistic, "
                                + stat_y + ", is invalid.");
        }
    }
}

template <typename Derived>
bool validator<Derived>::operator()(
    const swagger::v1::model::DynamicResultsConfig& config,
    std::vector<std::string>& errors)
{
    const auto& tholds =
        const_cast<swagger::v1::model::DynamicResultsConfig&>(config)
            .getThresholds();
    std::for_each(std::begin(tholds), std::end(tholds), [&](const auto& thold) {
        is_valid<Derived, swagger::v1::model::ThresholdConfig>(*thold, errors);
    });

    const auto& digests =
        const_cast<swagger::v1::model::DynamicResultsConfig&>(config)
            .getTdigests();
    std::for_each(
        std::begin(digests), std::end(digests), [&](const auto& digest) {
            is_valid<Derived, swagger::v1::model::TDigestConfig>(*digest,
                                                                 errors);
        });

    return (errors.empty());
}

} // namespace openperf::dynamic
