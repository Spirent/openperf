#include "internal_server_stack.hpp"

#include "framework/utils/overloaded_visitor.hpp"

namespace openperf::network::internal {

std::vector<server_stack::server_ptr> server_stack::list() const
{
    std::vector<server_ptr> network_generators_list;
    std::transform(m_servers.begin(),
                   m_servers.end(),
                   std::back_inserter(network_generators_list),
                   [](const auto& i) { return i.second; });

    return network_generators_list;
}

tl::expected<server_stack::server_ptr, std::string>
server_stack::create(const model::server& model)
{
    if (m_servers.count(model.id()))
        return tl::make_unexpected("Generator with ID " + model.id()
                                   + " already exists.");

    try {
        auto server_ptr = std::make_shared<class server>(model);
        m_servers.emplace(server_ptr->id(), server_ptr);
        return server_ptr;
    } catch (const std::runtime_error& e) {
        return tl::make_unexpected(e.what());
    }
}

server_stack::server_ptr server_stack::server(const std::string& id) const
{
    if (auto it = m_servers.find(id); it != m_servers.end()) return it->second;

    return nullptr;
}

bool server_stack::erase(const std::string& id)
{
    try {
        return m_servers.erase(id);
    } catch (const std::out_of_range&) {
        return false;
    }
}

} // namespace openperf::network::internal
