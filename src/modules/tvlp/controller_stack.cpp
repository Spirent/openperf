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
    if (get(model.id()))
        return tl::make_unexpected("TVLP configuration with id \""
                                   + static_cast<std::string>(model.id())
                                   + "\" already exists.");
    try {
        auto controller = std::make_shared<controller_t>(model);
        m_controllers.emplace(controller->id(), controller);
        return controller;
    } catch (const std::runtime_error& e) {
        return tl::make_unexpected(e.what());
    }
}

tl::expected<tvlp_controller_ptr, std::string>
controller_stack::get(const std::string& id) const
{
    if (m_controllers.count(id)) return m_controllers.at(id);
    return tl::make_unexpected("TVLP configuration with id \"" + id
                               + "\" not found.");
}
bool controller_stack::erase(const std::string& id) { return false; }

tl::expected<void, std::string>
controller_stack::start(const std::string& id, const time_point& start_time)
{
    auto controller = get(id);
    if (!controller) return tl::make_unexpected(controller.error());

    /*if (gen.value()->is_running()) {
        return tl::make_unexpected("Generator is already in running state");
    }*/

    controller.value()->start(start_time);

    // auto result = controller.value()->start();
    // m_block_results[result->get_id()] = gen;
    // return result;
    return {};
}

} // namespace openperf::tvlp::internal