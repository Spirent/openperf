#include <unistd.h>

#include "dynamic/validator.tcc"
#include "memory/api.hpp"
#include "memory/arg_parser.hpp"

#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryInfoResult.h"

namespace openperf::memory::api {

std::optional<long> get_sysconf_value(int key)
{
    auto result = sysconf(key);
    return (result == -1 ? std::nullopt : std::optional<long>(result));
}

/* Note: overflow will produce a negative value, so clamp out memory output. */
std::optional<long> get_total_memory()
{
    auto page_size = get_sysconf_value(_SC_PAGE_SIZE);
    auto phys_pages = get_sysconf_value(_SC_PHYS_PAGES);

    if (page_size && phys_pages) {
        return (std::clamp(page_size.value() * phys_pages.value(),
                           0L,
                           std::numeric_limits<long>::max()));
    }

    return (std::nullopt);
}

std::optional<long> get_available_memory()
{
    auto page_size = get_sysconf_value(_SC_PAGE_SIZE);
    auto avail_pages = get_sysconf_value(_SC_AVPHYS_PAGES);

    if (page_size && avail_pages) {
        return (std::clamp(page_size.value() * avail_pages.value(),
                           0L,
                           std::numeric_limits<long>::max()));
    }

    return (std::nullopt);
}

static void is_valid(const swagger::v1::model::MemoryGeneratorConfig& config,
                     std::vector<std::string>& errors)
{
    if (config.getBufferSize() < 64) {
        errors.emplace_back(
            "Buffer size must be greater or equal to 64 bytes.");
    }

    if (auto avail = get_available_memory()) {
        if (avail.value() < config.getBufferSize()) {
            errors.emplace_back(
                "Buffer size must be less than available memory of "
                + std::to_string(avail.value()) + " bytes.");
        }
    }

    if (to_pattern_type(config.getPattern()) == pattern_type::none) {
        errors.emplace_back("Pattern type \"" + config.getPattern()
                            + "\" is not recognized.");
    }

    if (config.getReadsPerSec() < 0) {
        errors.emplace_back("Reads per second cannot be negative.");
    }

    if (config.getReadSize() < 0) {
        errors.emplace_back("Read size cannot be negative.");
    }

    if (config.getReadsPerSec() && !config.getReadSize()) {
        errors.emplace_back("Cannot perform 0 byte read operations.");
    }

    auto mask = config::core_mask();
    auto nb_read_threads = config.getReadThreads();
    if (nb_read_threads < 0) {
        errors.emplace_back("The number of reader threads cannot be negative.");
    } else if (mask && mask->count() < static_cast<size_t>(nb_read_threads)) {
        errors.emplace_back("Not enough CPU cores available for readers: have "
                            + std::to_string(mask->count()) + ", need "
                            + std::to_string(nb_read_threads) + ".");
    }

    if (config.getWritesPerSec() < 0) {
        errors.emplace_back("Writes per second cannot be negative.");
    }

    if (config.getWriteSize() < 0) {
        errors.emplace_back("Write size cannot be negative.");
    }

    if (config.getWritesPerSec() && !config.getWriteSize()) {
        errors.emplace_back("Cannot perform 0 byte write operations.");
    }

    auto nb_write_threads = config.getWriteThreads();
    if (nb_write_threads < 0) {
        errors.emplace_back("The number of writer threads cannot be negative.");
    } else if (mask && mask->count() < static_cast<size_t>(nb_write_threads)) {
        errors.emplace_back("Not enough CPU cores available for writers: have "
                            + std::to_string(mask->count()) + ", need "
                            + std::to_string(nb_write_threads) + ".");
    }
}

bool is_valid(const mem_generator_type& generator,
              std::vector<std::string>& errors)
{
    auto config = generator.getConfig();
    if (!config) {
        errors.emplace_back("Generator configuration is required.");
        return (false);
    }

    is_valid(*config, errors);

    return (errors.empty());
}

template <typename T, typename... Items>
constexpr auto make_array(Items&&... items) -> std::array<T, sizeof...(items)>
{
    return {{std::forward<Items>(items)...}};
}

struct memory_dynamic_validator
    : public dynamic::validator<memory_dynamic_validator>
{

    static constexpr auto valid_mem_io_stats =
        make_array<std::string_view>("bytes_actual",
                                     "bytes_target",
                                     "latency_total",
                                     "ops_actual",
                                     "ops_target");

    static bool is_valid_stat(std::string_view name)
    {
        auto is_valid_io_name = [](std::string_view name) -> bool {
            auto cursor = std::begin(valid_mem_io_stats),
                 end = std::end(valid_mem_io_stats);
            while (cursor != end) {
                if (*cursor == name) { return (true); }
                cursor++;
            }

            return (false);
        };

        if (name == "timestamp") { return (true); }

        constexpr std::string_view read_prefix = "read.";
        constexpr std::string_view write_prefix = "write.";

        if (name.substr(0, read_prefix.length()) == read_prefix) {
            auto stat_name = name.substr(read_prefix.length());
            return (is_valid_io_name(stat_name));
        } else if (name.substr(0, write_prefix.length()) == write_prefix) {
            auto stat_name = name.substr(write_prefix.length());
            return (is_valid_io_name(stat_name));
        }

        return (false);
    };
};

bool is_valid(const swagger::v1::model::DynamicResultsConfig& config,
              std::vector<std::string>& errors)
{
    return (memory_dynamic_validator{}(config, errors));
}

mem_info_ptr get_memory_info()
{
    auto info = std::make_unique<swagger::v1::model::MemoryInfoResult>();

    if (auto total = get_total_memory()) {
        info->setTotalMemory(total.value());
    }

    if (auto avail = get_available_memory()) {
        info->setFreeMemory(avail.value());
    }

    return (info);
}

} // namespace openperf::memory::api

template struct openperf::dynamic::validator<
    openperf::memory::api::memory_dynamic_validator>;
