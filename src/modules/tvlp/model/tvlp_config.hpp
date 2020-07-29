#ifndef _OP_TVLP_CONFIG_MODEL_HPP_
#define _OP_TVLP_CONFIG_MODEL_HPP_

#include <optional>
#include <string>

namespace openperf::tvlp::model {

class tvlp_configuration_t
{
public:
    tvlp_configuration_t() = default;
    tvlp_configuration_t(const tvlp_configuration_t&) = default;

    inline std::string get_id() const { return m_id; };
    inline void set_id(std::string_view value) { m_id = value; };

protected:
    std::string m_id;
};

} // namespace openperf::tvlp::model

#endif // _OP_TVLP_CONFIG_MODEL_HPP_