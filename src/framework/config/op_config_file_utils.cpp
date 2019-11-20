#include "op_config_file_utils.hpp"
#include "yaml_json_emitter.hpp"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/emitfromevents.h"
#include "../src/nodeevents.h"

#include <sstream>
#include <tuple>
#include <cstdlib>
#include <iostream>

namespace openperf::config::file {

bool op_config_is_number(std::string_view value)
{
    if (value.empty()) { return false; }

    char *endptr;

    // Is it an integer?
    strtoll(value.data(), &endptr, 0);
    if (value.data() + value.length() == endptr) { return true; }

    // How about a double?
    strtod(value.data(), &endptr);
    if (value.data() + value.length() == endptr) { return true; }

    return false;
}

std::tuple<std::string_view, std::string_view> op_config_split_path_id(std::string_view path_id)
{
    size_t last_slash_location = path_id.find_last_of('/');
    if (last_slash_location == std::string_view::npos) {
        // No slash. Not a valid /path/id string.
        return std::tuple(std::string_view(), std::string_view());
    }

    if (last_slash_location == 0) { return std::tuple(path_id, std::string_view()); }

    return std::tuple(path_id.substr(0, last_slash_location),
                      path_id.substr(last_slash_location + 1));
}

tl::expected<std::string, std::string> op_config_yaml_to_json(const YAML::Node &yaml_src)
{
    std::ostringstream output_stream;
    yaml_json_emitter emitter(output_stream);
    NodeEvents events(yaml_src);

    try {
        events.Emit(emitter);
    } catch (std::exception &e) {
        return(tl::make_unexpected("Error while converting YAML configuration to JSON. " + std::string(e.what())));
    }

    return output_stream.str();
}

}  // namespace openperf::config::file
