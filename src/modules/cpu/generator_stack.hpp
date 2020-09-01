#ifndef _OP_CPU_GENERATOR_STACK_HPP_
#define _OP_CPU_GENERATOR_STACK_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include <tl/expected.hpp>

#include "generator.hpp"

#include "modules/dynamic/api.hpp"

namespace openperf::cpu::generator {

class generator_stack
{
private:
    using generator_ptr = std::shared_ptr<generator>;
    using statistic_variant =
        std::variant<generator_ptr, model::generator_result>;

private:
    std::unordered_map<std::string, generator_ptr> m_generators;
    std::unordered_map<std::string, statistic_variant> m_statistics;

public:
    generator_stack() = default;

    tl::expected<generator_ptr, std::string> create(const model::generator&);

    generator_ptr generator(const std::string&) const;
    std::vector<generator_ptr> list() const;
    bool erase(const std::string&);

    tl::expected<model::generator_result, std::string>
    statistics(const std::string&) const;
    std::vector<model::generator_result> list_statistics() const;
    bool erase_statistics(const std::string&);

    tl::expected<model::generator_result, std::string>
    start_generator(const std::string& id);
    tl::expected<model::generator_result, std::string>
    start_generator(const std::string& id, const dynamic::configuration&);
    bool stop_generator(const std::string& id);
};

} // namespace openperf::cpu::generator

#endif // _OP_CPU_GENERATOR_STACK_HPP_
