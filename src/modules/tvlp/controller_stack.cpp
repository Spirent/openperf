#include "controller_stack.hpp"

namespace openperf::tvlp::internal {

std::vector<tvlp_controller_ptr> controller_stack::list() const
{
    std::vector<tvlp_controller_ptr> controllers_list;
    for (const auto& c_pair : m_controllers) {
        c_pair.second->update();
        controllers_list.push_back(c_pair.second);
    }

    return controllers_list;
}

tl::expected<tvlp_controller_ptr, std::string>
controller_stack::create(const model::tvlp_configuration_t& model)
{
    if (get(model.id())) {
        return tl::make_unexpected("TVLP configuration with id \""
                                   + static_cast<std::string>(model.id())
                                   + "\" already exists.");
    }

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
    if (m_controllers.count(id)) {
        auto controller = m_controllers.at(id);
        controller->update();
        return controller;
    }
    return tl::make_unexpected("TVLP configuration with id \"" + id
                               + "\" not found.");
}
tl::expected<void, std::string> controller_stack::erase(const std::string& id)
{
    auto controller = get(id);
    if (!controller) {
        return tl::make_unexpected("TVLP configuration with id \"" + id
                                   + "\" not found.");
    }

    if (controller.value()->is_running()) {
        return tl::make_unexpected("Cannot delete TVLP configuration "
                                   "in running state.");
    }

    m_controllers.erase(id);

    return {};
}

tl::expected<tvlp_result_ptr, std::string>
controller_stack::start(const std::string& id, const time_point& start_time)
{
    auto controller = get(id);
    if (!controller) return tl::make_unexpected(controller.error());

    if (controller.value()->is_running()) {
        return tl::make_unexpected("Generator is already in running state");
    }

    auto result = controller.value()->start(start_time);
    m_results[result->id()] = result;
    return result;
}

tl::expected<void, std::string> controller_stack::stop(const std::string& id)
{
    auto controller = get(id);
    if (!controller) return tl::make_unexpected(controller.error());

    if (!controller.value()->is_running()) {
        return tl::make_unexpected("Generator is not in running state");
    }

    controller.value()->stop();

    return {};
}

std::vector<tvlp_result_ptr> controller_stack::results() const
{
    std::vector<tvlp_result_ptr> result_list;
    for (const auto& pair : m_results) {
        get(pair.second->tvlp_id());
        result_list.push_back(pair.second);
    }
    return result_list;
}

tl::expected<tvlp_result_ptr, std::string>
controller_stack::result(const std::string& id) const
{
    if (!m_results.count(id)) {
        return tl::make_unexpected("TVLP result with id \"" + id
                                   + "\" not found.");
    }

    auto result = m_results.at(id);
    get(result->tvlp_id());
    return result;
}

tl::expected<void, std::string>
controller_stack::erase_result(const std::string& id)
{
    auto res = result(id);
    if (!res) {
        return tl::make_unexpected("TVLP result with id \"" + id
                                   + "\" not found.");
    }

    auto controller = get(res.value()->tvlp_id());
    if (controller && controller.value()->is_running()) {
        return tl::make_unexpected(
            "Cannot delete TVLP result in running state.");
    }

    m_results.erase(id);

    return {};
}

} // namespace openperf::tvlp::internal