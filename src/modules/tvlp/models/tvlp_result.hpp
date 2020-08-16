#ifndef _OP_TVLP_RESULT_MODEL_HPP_
#define _OP_TVLP_RESULT_MODEL_HPP_

#include <optional>
#include <string>
#include "json.hpp"

namespace openperf::tvlp::model {

using json_vector = std::vector<nlohmann::json>;

struct tvlp_modules_results_t
{
    std::optional<json_vector> block;
    std::optional<json_vector> cpu;
    std::optional<json_vector> memory;
    std::optional<json_vector> packet;
};

class tvlp_result_t
{
public:
    tvlp_result_t() = default;
    tvlp_result_t(const tvlp_result_t&) = default;

    inline std::string id() const { return m_id; };
    inline void id(std::string_view value) { m_id = value; };

    inline std::string tvlp_id() const { return m_tvlp_id; };
    inline void tvlp_id(std::string_view value) { m_tvlp_id = value; };

    inline tvlp_modules_results_t results() const { return m_results; };
    inline void results(const tvlp_modules_results_t& value)
    {
        m_results = value;
    };

protected:
    std::string m_id = "";
    std::string m_tvlp_id = "";

    tvlp_modules_results_t m_results;
};

} // namespace openperf::tvlp::model

#endif // _OP_TVLP_RESULT_MODEL_HPP_