#ifndef _OP_CPU_GENERATOR_RESULT_MODEL_HPP_
#define _OP_CPU_GENERATOR_RESULT_MODEL_HPP_

#include <string_view>

#include "modules/timesync/chrono.hpp"
#include "cpu/cpu_stat.hpp"

namespace openperf::cpu::model {

class generator_result
{
private:
    using time_point = timesync::chrono::realtime::time_point;

public:
    generator_result() = default;
    generator_result(const generator_result&) = default;

    std::string id() const { return m_id; }
    void id(std::string_view id) { m_id = id; }

    std::string generator_id() const { return m_generator_id; }
    void generator_id(std::string_view id) { m_generator_id = id; }

    bool active() const { return m_active; }
    void active(bool active) { m_active = active; }

    time_point timestamp() const { return m_timestamp; }
    void timestamp(const time_point& time) { m_timestamp = time; }

    cpu_stat stats() const { return m_stats; }
    void stats(const cpu_stat& stats) { m_stats = stats; }

    double field(std::string_view name) const
    {
        if (name == "available") return available.count();
        if (name == "utilization") return utilization.count();
        if (name == "system") return system.count();
        if (name == "user") return user.count();
        if (name == "steal") return steal.count();
        if (name == "error") return error.count();

        constexpr auto prefix = "cores[";
        auto prefix_size = std::strlen(prefix);
        if (name.substr(0, prefix_size) == prefix) {
            auto core_number = std::stoul(std::string(name.substr(
                prefix_size,
                name.find_first_of(']', prefix_size) - prefix_size)));

            if (core_number < cores.size()) {
                auto field_name =
                    name.substr(name.find_first_of('.', prefix_size) + 1);

                if (field_name == "available")
                    return cores[core_number].available.count();
                if (field_name == "utilization")
                    return cores[core_number].utilization.count();
                if (field_name == "system")
                    return cores[core_number].system.count();
                if (field_name == "user")
                    return cores[core_number].user.count();
                if (field_name == "steal")
                    return cores[core_number].steal.count();
                if (field_name == "error")
                    return cores[core_number].error.count();
            }
        }

        return 0.0;
    }

    bool has_field(std::string_view name) const
    {
        const std::set<std::string> fields = {
            "available", "utilization", "system", "user", "steal", "error"};

        if (fields.count(std::string(name))) return true;

        constexpr auto prefix = "cores[";
        auto prefix_size = std::strlen(prefix);
        if (name.substr(0, prefix_size) == prefix) {
            auto core_number = std::stoul(std::string(name.substr(
                prefix_size,
                name.find_first_of(']', prefix_size) - prefix_size)));

            if (core_number >= cores.size()) return false;

            auto field_name = std::string(
                name.substr(name.find_first_of('.', prefix_size) + 1));

            return fields.count(field_name);
        }

        return false;
    }
};

class generator_result
{
public:
    generator_result() = default;
    generator_result(const generator_result&) = default;

    std::string id() const { return m_id; }
    void id(std::string_view id) { m_id = id; }

    std::string generator_id() const { return m_generator_id; }
    void generator_id(std::string_view id) { m_generator_id = id; }

    bool active() const { return m_active; }
    void active(bool active) { m_active = active; }

    generator_stats stats() const { return m_stats; }
    void stats(const generator_stats& stats) { m_stats = stats; }

    dynamic::results dynamic_results() const { return m_dynamic_results; }
    void dynamic_results(const dynamic::results& r) { m_dynamic_results = r; }

protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    cpu_stat m_stats;
    dynamic::results m_dynamic_results;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_GENERATOR_RESULT_MODEL_HPP_