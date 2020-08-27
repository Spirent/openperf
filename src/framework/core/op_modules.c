#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <regex.h>
#include <errno.h>
#include <dlfcn.h>
#include <linux/limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "core/op_log.h"
#include "core/op_modules.h"
#include "core/op_common.h"
#include "core/op_options.h"

static SLIST_HEAD(op_modules_list, op_module)
    op_modules_list_head = SLIST_HEAD_INITIALIZER(op_modules_list_head);

static const size_t op_modules_err_str_max_len = 144;

char op_plugin_modules_path[PATH_MAX];
static int op_find_plugin_modules_path()
{
    char *p, path[PATH_MAX];
    int rv;

    /* find executable path */
    if ((rv = readlink("/proc/self/exe", path, PATH_MAX - 1)) == -1) return 1;

    /* readlink doesn't provide null termination */
    path[rv] = 0;

    /* strip filename */
    if ((p = strrchr(path, '/')) == 0) return 1;
    *p = 0;

    /* strip bin/ */
    if ((p = strrchr(path, '/')) == 0) return 1;
    *p = 0;

    strcpy(op_plugin_modules_path, path);
    strcat(op_plugin_modules_path, "/plugins");

    return 0;
}

static int plugin_option_handler(int opt __attribute__((unused)),
                                 const char* opt_arg)
{
    if (opt_arg) { strcpy(op_plugin_modules_path, opt_arg); }

    return (0);
}

static struct op_options_data plugin_options = {
    .name = "PLUGIN",
    .init = op_find_plugin_modules_path,
    .callback = plugin_option_handler,
    .options =
        {
            {"Specifies path to plugin modules",
             "plugin",
             'm',
             OP_OPTION_TYPE_STRING},
            {0, 0, 0, 0},
        },
};
REGISTER_OPTIONS(plugin_options)

void op_modules_load()
{
    void* handle;
    struct op_module* reg;
    DIR* dir;
    struct dirent* entry;
    struct stat statb;

    OP_LOG(OP_LOG_INFO, "Plugin modules path %s\n", op_plugin_modules_path);

    dir = opendir(op_plugin_modules_path);

    if (dir == 0) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to open plugin modules path: %s\n",
               op_plugin_modules_path);
        return;
    }

    while ((entry = readdir(dir))) {
        char path[PATH_MAX];
        sprintf(path, "%s/%s", op_plugin_modules_path, entry->d_name);

        /* Only accept .so */
        char* ext = strrchr((const char*)path, '.');
        /* unreadable */
        if (!ext || (strcmp(ext, ".so") != 0) || stat((char*)path, &statb) < 0)
            continue;

        /* a dir or other things which aren't plugins */
        if (!S_ISREG(statb.st_mode)) continue;

        handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);

        if (handle == 0) {
            OP_LOG(OP_LOG_ERROR, "Failed to load plugin: %s\n", dlerror());
            continue;
        }
        reg = dlsym(handle, "op_plugin_module_registration");
        if (reg == 0) {
            /* This should never happen unless registration process was changed
             */
            OP_LOG(OP_LOG_ERROR, "Missing module registration in plugin \n");
            dlclose(handle);
            continue;
        }

        OP_LOG(OP_LOG_INFO,
               "Loaded plugin module %s (%s, %s)\n",
               entry->d_name,
               reg->info.id,
               reg->info.description);
    }

    closedir(dir);
}

void op_modules_register(struct op_module* module)
{
    /* Verify required module information fields are provided. */
    assert(module->info.id); /* also try to catch init order problems */
    assert(module->info.description);
    assert(module->info.linkage);

    /**
     * Since we allow end users to get module information by
     *  id via a URI, make sure module id is valid
     *  for inclusion in a URI.
     * For simplicity this is a subset of what RFC-3986 allows.
     */
    regex_t regex;

    int regresult = regcomp(&regex, OP_MODULE_ID_REGEX, REG_EXTENDED);
    if (regresult) {
        char error_string[op_modules_err_str_max_len];
        size_t error_string_len = regerror(
            regresult, &regex, error_string, op_modules_err_str_max_len);
        op_exit(
            "Module registration id validation regex failed to compile: %s%s\n",
            error_string,
            error_string_len == op_modules_err_str_max_len ? "(truncated)"
                                                           : "");
    }

    regresult = regexec(&regex, module->info.id, 0, NULL, 0);
    if (regresult == REG_NOMATCH) {
        op_exit("During Module registration found invalid id %s.\n",
                module->info.id);
    }

    regfree(&regex);

    SLIST_INSERT_HEAD(&op_modules_list_head, module, next);
}

int op_modules_pre_init(void* context)
{
    struct op_module* module = NULL;
    int errors = 0;

    SLIST_FOREACH (module, &op_modules_list_head, next) {
        if (!module->pre_init) { continue; }
        int ret = module->pre_init(context, module->state);
        errors += !!(ret != 0);
        OP_LOG(OP_LOG_INFO,
               "Pre-initializing %s module: %s\n",
               module->info.id,
               ret ? "Failed" : "OK");
    }

    return (errors);
}

int op_modules_init(void* context)
{
    struct op_module* module = NULL;
    int errors = 0;

    SLIST_FOREACH (module, &op_modules_list_head, next) {
        if (!module->init) { continue; }
        int ret = module->init(context, module->state);
        errors += !!(ret != 0);
        OP_LOG(OP_LOG_INFO,
               "Initializing %s module: %s\n",
               module->info.id,
               ret ? "Failed" : "OK");
    }

    return (errors);
}

int op_modules_post_init(void* context)
{
    struct op_module* module = NULL;
    int errors = 0;

    SLIST_FOREACH (module, &op_modules_list_head, next) {
        if (!module->post_init) { continue; }
        int ret = module->post_init(context, module->state);
        errors += !!(ret != 0);
        OP_LOG(OP_LOG_INFO,
               "Post-initializing %s module: %s\n",
               module->info.id,
               ret ? "Failed" : "OK");
    }

    return (errors);
}

int op_modules_start()
{
    struct op_module* module = NULL;
    int errors = 0;

    SLIST_FOREACH (module, &op_modules_list_head, next) {
        if (!module->start) { continue; }
        int ret = module->start(module->state);
        errors += !!(ret != 0);
        OP_LOG(OP_LOG_INFO,
               "Starting %s module: %s\n",
               module->info.id,
               ret ? "Failed" : "OK");
    }

    return (errors);
}

void op_modules_finish()
{
    struct op_module* module = NULL;
    SLIST_FOREACH (module, &op_modules_list_head, next) {
        if (!module->finish) { continue; }
        module->finish(module->state);
    }
}

size_t op_modules_get_loaded_count()
{
    int loaded_module_count = 0;
    struct op_module* module = NULL;

    SLIST_FOREACH (module, &op_modules_list_head, next) {
        loaded_module_count++;
    }

    return loaded_module_count;
}

const struct op_module_info* op_modules_get_info_by_id(const char* module_id)
{
    if (!module_id) return NULL;

    struct op_module* module = NULL;
    SLIST_FOREACH (module, &op_modules_list_head, next) {

        if (strcmp(module_id, module->info.id) == 0) { return &module->info; }
    }

    return NULL;
}

int op_modules_get_info_list(const struct op_module_info* info[],
                             size_t max_entries)
{
    if (!info) return -1;

    if (max_entries == 0) return op_modules_get_loaded_count();

    size_t module_list_count = 0;
    struct op_module* module = NULL;
    SLIST_FOREACH (module, &op_modules_list_head, next) {
        info[module_list_count] = &module->info;
        module_list_count++;

        if (module_list_count == max_entries) return module_list_count;
    }

    return module_list_count;
}
