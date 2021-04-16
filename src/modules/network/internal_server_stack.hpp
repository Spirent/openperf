#ifndef _OP_NETWORK_INTERNAL_SERVER_STACK_HPP_
#define _OP_NETWORK_INTERNAL_SERVER_STACK_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include <tl/expected.hpp>

namespace openperf::network {

namespace model {
class server;
}

namespace internal {

class server;

class server_stack
{
private:
    using server_ptr = std::shared_ptr<server>;

private:
    std::unordered_map<std::string, server_ptr> m_servers;

public:
    server_stack() = default;

    tl::expected<server_ptr, std::string> create(const model::server&);

    server_ptr server(const std::string&) const;
    std::vector<server_ptr> list() const;
    bool erase(const std::string&);
};

} // namespace internal
} // namespace openperf::network

#endif // _OP_CPU_GENERATOR_STACK_HPP_
