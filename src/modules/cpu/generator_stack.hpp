#ifndef _OP_CPU_GENERATOR_STACK_HPP_
#define _OP_CPU_GENERATOR_STACK_HPP_

#include <unordered_map>
#include <variant>

#include "generator.hpp"
#include "tl/expected.hpp"

namespace openperf::cpu::generator {

class generator_stack
{
public:
    struct statistic_t {
        std::string id;
        std::string generator_id;
        model::generator_result statistics;
    };

private:
    using generator_ptr = std::shared_ptr<generator>;
    using statistic_variant = std::variant<generator_ptr, statistic_t>;

private:
    std::unordered_map<std::string, generator_ptr> m_generators;
    std::unordered_map<std::string, statistic_variant> m_statistics;
    std::unordered_map<std::string, std::string> m_id_map;

public:
    generator_stack() = default;

    tl::expected<generator_ptr, std::string> create(const model::generator&);
    void erase(const std::string&);
    bool erase_statistics(const std::string&);

    generator_ptr generator(const std::string&) const;
    statistic_t statistics(const std::string&) const;
    std::vector<generator_ptr> list() const;
};

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_STACK_HPP_ */