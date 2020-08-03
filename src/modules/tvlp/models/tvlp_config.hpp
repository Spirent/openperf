#ifndef _OP_TVLP_CONFIG_MODEL_HPP_
#define _OP_TVLP_CONFIG_MODEL_HPP_

#include <optional>
#include <string>
#include "json.hpp"

namespace openperf::tvlp::model {

struct tvlp_profile_entry_t
{
    uint64_t length = 0;
    std::optional<std::string> resource_id;
    nlohmann::json config;
};

struct tvlp_profile_t
{
    std::optional<std::vector<tvlp_profile_entry_t>> block;
    std::optional<std::vector<tvlp_profile_entry_t>> cpu;
    std::optional<std::vector<tvlp_profile_entry_t>> memory;
    std::optional<std::vector<tvlp_profile_entry_t>> packet;
};

class tvlp_configuration_t
{
public:
    enum state { READY = 0, COUNTDOWN, RUNNING, ERROR };

    tvlp_configuration_t() = default;
    tvlp_configuration_t(const tvlp_configuration_t&) = default;

    inline std::string id() const { return m_id; };
    inline void id(std::string_view value) { m_id = value; };

    inline tvlp_profile_t profile() const { return m_profile; };
    inline void profile(const tvlp_profile_t& value) { m_profile = value; };

    inline state state() const { return m_state; };
    inline std::string start_time() const { return m_start_time; };
    inline uint64_t total_length() const { return m_total_length; };
    inline uint64_t current_offset() const { return m_current_offset; };

    inline std::string error() const { return m_error; };

protected:
    std::string m_id = "";
    enum state m_state = READY;

    std::string m_start_time = "";
    uint64_t m_total_length = 0;
    uint64_t m_current_offset = 0;

    tvlp_profile_t m_profile;
    std::string m_error = "";
};

} // namespace openperf::tvlp::model

#endif // _OP_TVLP_CONFIG_MODEL_HPP_