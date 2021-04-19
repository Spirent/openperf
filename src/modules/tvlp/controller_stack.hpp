#ifndef _OP_TVLP_CONTROLLER_STACK_HPP_
#define _OP_TVLP_CONTROLLER_STACK_HPP_

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <variant>

#include "tl/expected.hpp"

//#include "controller.hpp"
//#include "models/tvlp_config.hpp"
//#include "models/tvlp_result.hpp"
//#include "tl/expected.hpp"

namespace openperf::tvlp {

namespace model {
class tvlp_configuration_t;
class tvlp_result_t;
struct tvlp_start_t;
} // namespace model

namespace internal {

class controller_t;

class controller_stack
{
    using tvlp_controller_ptr = std::shared_ptr<controller_t>;
    using tvlp_controller_map =
        std::unordered_map<std::string, tvlp_controller_ptr>;
    using tvlp_result_ptr = std::shared_ptr<model::tvlp_result_t>;
    using tvlp_result_map = std::unordered_map<std::string, tvlp_result_ptr>;

private:
    tvlp_controller_map m_controllers;
    tvlp_result_map m_results;
    void* m_context;

public:
    controller_stack() = delete;
    controller_stack(void* context);

    std::vector<tvlp_controller_ptr> list() const;
    tl::expected<tvlp_controller_ptr, std::string>
    create(const model::tvlp_configuration_t&);
    tl::expected<tvlp_controller_ptr, std::string>
    get(const std::string& id) const;
    tl::expected<void, std::string> erase(const std::string& id);

    tl::expected<tvlp_result_ptr, std::string>
    start(const std::string&, const model::tvlp_start_t&);
    tl::expected<void, std::string> stop(const std::string&);

    std::vector<tvlp_result_ptr> results() const;
    tl::expected<tvlp_result_ptr, std::string>
    result(const std::string& id) const;
    tl::expected<void, std::string> erase_result(const std::string& id);
};

} // namespace internal
} // namespace openperf::tvlp

#endif /* _OP_TVLP_CONTROLLER_STACK_HPP_ */
