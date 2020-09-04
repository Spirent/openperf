#include <assert.h>
#include <dlfcn.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <filesystem>

#include "core/op_log.h"
#include "core/op_modules.h"
#include "core/op_options.h"
#include "config/op_config_file.hpp"

namespace openperf::core {

static std::optional<std::filesystem::path>
find_plugin_modules_path(int argc, char* const argv[])
{
    auto path = std::filesystem::canonical(std::filesystem::path(argv[0]))
                    .remove_filename();
    path += "../plugins";
    path = std::filesystem::canonical(path);

    if (!std::filesystem::exists(path)) return std::nullopt;

    return path;
}

static std::optional<std::filesystem::path> find_plugin_path(int argc,
                                                             char* const argv[])
{
    for (int idx = 0; idx < argc - 1; idx++) {
        if (strcmp(argv[idx], "--modules.plugins.path") == 0
            || strcmp(argv[idx], "-m") == 0) {
            auto path = std::filesystem::path(argv[idx + 1]);
            if (!std::filesystem::exists(path)) return std::nullopt;
            return path;
        }
    }

    /* Check if configuration file has a plugin path option. */
    char arg_string[PATH_MAX];
    if (op_config_file_get_value_str(
            "modules.plugins.path", arg_string, PATH_MAX)) {
        auto path =
            std::filesystem::canonical(std::filesystem::path(arg_string));
        if (!std::filesystem::exists(path)) return std::nullopt;
        return path;
    }

    return find_plugin_modules_path(argc, argv);
}

std::optional<std::vector<std::string>>
find_plugins_files_list_option(int argc, char* const argv[])
{
    for (int idx = 0; idx < argc - 1; idx++) {
        if (strcmp(argv[idx], "--modules.plugins.load") == 0
            || strcmp(argv[idx], "-L") == 0) {
            std::vector<std::string> res;
            std::string p;

            auto plugins = std::string(argv[idx + 1]);
            std::stringstream ss(plugins);
            while (std::getline(ss, p, ',')) { res.push_back(p); }
            return res;
        }
    }

    /* Check if configuration file has a plugin files list option. */
    return config::file::op_config_get_param<OP_OPTION_TYPE_LIST>(
        "modules.plugins.load");
}

extern "C" {

void op_modules_load(int argc, char* const argv[])
{
    auto plugin_files = find_plugins_files_list_option(argc, argv);
    if (!plugin_files) return;

    auto plugin_modules_path = find_plugin_path(argc, argv);
    if (!plugin_modules_path) {
        OP_LOG(OP_LOG_CRITICAL, "Plugins path does not exists\n");
        assert(false);
    }

    OP_LOG(OP_LOG_INFO,
           "Plugin modules path %s\n",
           plugin_modules_path.value().c_str());

    for (auto& entry : plugin_files.value()) {
        auto path = plugin_modules_path.value();
        path += std::string("/") + entry;
        auto handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (handle == 0) {
            OP_LOG(OP_LOG_CRITICAL, "Failed to load plugin: %s\n", dlerror());
            assert(false);
        }

        auto reg = (op_module*)dlsym(handle, "op_plugin_module_registration");
        if (reg == 0) {
            /* This should never happen unless registration process was changed
             */
            OP_LOG(OP_LOG_ERROR, "Missing module registration in plugin \n");
            dlclose(handle);
            assert(false);
        }

        OP_LOG(OP_LOG_INFO,
               "Loaded plugin module %s (%s, %s)\n",
               entry.c_str(),
               reg->info.id,
               reg->info.description);
    }
}
}

} // namespace openperf::core
