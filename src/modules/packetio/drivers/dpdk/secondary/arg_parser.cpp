#include <stack>

#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/secondary/arg_parser.hpp"

namespace openperf::packetio::dpdk::config {

using namespace openperf::config;

inline constexpr std::string_view proc_type = "--proc-type";
inline constexpr std::string_view proc_opt = "secondary";

std::vector<std::string> dpdk_args()
{
    auto args = common_dpdk_args();
    if (!contains(args, proc_type)) {
        args.emplace_back(proc_type);
        args.emplace_back(proc_opt);
    }

    return (args);
}

static std::optional<uint16_t> to_uint16(std::string_view input)
{
    std::stringstream ss(std::string(input), std::ios_base::in);
    uint16_t out;
    if ((ss >> out).fail()) { return (std::nullopt); }
    return (out);
}

static std::vector<uint16_t> parse_queues(std::string_view input)
{
    static constexpr auto delimiters = ",-";
    auto queues = std::vector<uint16_t>{};
    auto tmp = std::stack<uint16_t>{};

    size_t cursor = 0, end = 0;
    while ((cursor = input.find_first_not_of(delimiters, end))
           != std::string::npos) {
        end = input.find_first_of(delimiters, cursor + 1);

        if (auto value = to_uint16(input.substr(cursor, end - cursor))) {
            if (end > input.length()) { /* no more input */
                if (tmp.empty()) {
                    queues.push_back(*value);
                } else {
                    for (uint16_t i = tmp.top(); i <= *value; i++) {
                        queues.push_back(i);
                    }
                    tmp.pop();
                }
            } else { /* check delimiter */
                switch (input[end]) {
                case '-':
                    tmp.push(*value);
                    break;
                case ',':
                    queues.push_back(*value);
                    break;
                default:
                    throw std::invalid_argument("Invalid queue syntax");
                }
            }
        }
    }

    return (queues);
}

// Split portX and return just the X part.
// Return -1 if no valid X part is found.
static std::optional<uint16_t> get_port_index(std::string_view name)
{
    auto index_offset = name.find_first_not_of("port");
    if (index_offset == std::string_view::npos) { return (std::nullopt); }

    return (to_uint16(name.substr(
        index_offset, name.find_first_not_of("0123456789", index_offset + 1))));
}

static std::map<uint16_t, std::vector<uint16_t>>
get_port_queues(std::string_view opt_name)
{
    auto map = std::map<uint16_t, std::vector<uint16_t>>{};

    auto opt_arg =
        config::file::op_config_get_param<OP_OPTION_TYPE_MAP>(opt_name);

    if (!opt_arg) { return (map); }

    for (auto&& pair : *opt_arg) {
        if (auto port_idx = get_port_index(pair.first)) {
            map[*port_idx] = parse_queues(pair.second);
        }
    }

    return (map);
}

std::map<uint16_t, std::vector<uint16_t>> tx_port_queues()
{
    return (get_port_queues(op_packetio_dpdk_tx_queues));
}

} // namespace openperf::packetio::dpdk::config
