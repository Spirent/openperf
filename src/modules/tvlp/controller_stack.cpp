#include "controller_stack.hpp"

namespace openperf::tvlp::internal {
std::vector<tvlp_controller_ptr> controller_stack::list() const
{
    std::vector<tvlp_controller_ptr> controllers_list;
    for (const auto& c_pair : m_controllers) {
        controllers_list.push_back(c_pair.second);
    }

    return controllers_list;
}
tl::expected<tvlp_controller_ptr, std::string>
controller_stack::create(const model::tvlp_configuration_t& model)
{
    return tl::make_unexpected("Generator " + model.id() + " already exists.");
}
tvlp_controller_ptr controller_stack::get(const std::string& id) const
{
    return nullptr;
}
bool controller_stack::erase(const std::string& id) { return false; }
} // namespace openperf::tvlp::internal