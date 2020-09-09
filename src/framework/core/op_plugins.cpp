#include <cassert>
#include <dlfcn.h>
#include <climits>
#include <dirent.h>
#include <sys/stat.h>
#include <filesystem>

#include "core/op_log.h"
#include "core/op_modules.h"
#include "core/op_options.h"
#include "config/op_config_file.hpp"

namespace openperf::core {

static std::optional<std::filesystem::path> find_plugin_path()
{
    return config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
        "modules.plugins.path");
}

static std::optional<std::vector<std::string>> find_plugins_files_list_option()
{
    return config::file::op_config_get_param<OP_OPTION_TYPE_LIST>(
        "modules.plugins.load");
}

extern "C" {

int op_modules_load()
{
    auto plugin_files = find_plugins_files_list_option();
    if (!plugin_files) return 0;

    auto plugin_modules_path = find_plugin_path();
    if (!plugin_modules_path) {
        OP_LOG(OP_LOG_CRITICAL, "Missing modules.plugins.path property\n");
        return 1;
    }

    OP_LOG(OP_LOG_INFO,
           "Plugin modules path %s\n",
           plugin_modules_path.value().c_str());

    for (auto& entry : plugin_files.value()) {
        auto path = plugin_modules_path.value();
        path += std::string("/") + entry;
        auto handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!handle) {
            OP_LOG(OP_LOG_CRITICAL, "Failed to load plugin: %s\n", dlerror());
            assert(false);
        }

        auto reg = (op_module*)dlsym(handle, "op_plugin_module_registration");
        if (!reg) {
            /* This should never happen unless registration process was changed
             */
            OP_LOG(OP_LOG_ERROR,
                   "Missing module registration in plugin %s\n",
                   entry.c_str());
            dlclose(handle);
            return 1;
        }

        OP_LOG(OP_LOG_INFO,
               "Loaded plugin module %s (%s, %s)\n",
               entry.c_str(),
               reg->info.id,
               reg->info.description);
    }

    return 0;
}
}

} // namespace openperf::core
