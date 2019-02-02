#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <regex.h>

#include "core/icp_log.h"
#include "core/icp_modules.h"

static SLIST_HEAD(icp_modules_list, icp_module) icp_modules_list_head =
    SLIST_HEAD_INITIALIZER(icp_modules_list_head);

void icp_modules_register(struct icp_module *module)
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
    int regresult = regcomp(&regex, ICP_MODULE_ID_REGEX, REG_EXTENDED);
    assert(!regresult);
    regresult = regexec(&regex, module->info.id, 0, NULL, 0);
    assert(!regresult);
    regfree(&regex);

    SLIST_INSERT_HEAD(&icp_modules_list_head, module, next);
}

int icp_modules_pre_init(void *context)
{
    struct icp_module *module = NULL;
    int errors = 0;

    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        if (!module->pre_init) {
            continue;
        }
        int ret = module->pre_init(context, module->state);
        errors += !!(ret != 0);
        ICP_LOG(ICP_LOG_INFO, "Pre-initializing %s module: %s\n",
                module->info.id, ret ? "Failed" : "OK");
    }

    return (errors);
}

int icp_modules_init(void *context)
{
    struct icp_module *module = NULL;
    int errors = 0;

    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        if (!module->init) {
            continue;
        }
        int ret = module->init(context, module->state);
        errors += !!(ret != 0);
        ICP_LOG(ICP_LOG_INFO, "Initializing %s module: %s\n",
                module->info.id, ret ? "Failed" : "OK");
    }

    return (errors);
}

int icp_modules_post_init(void *context)
{
    struct icp_module *module = NULL;
    int errors = 0;

    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        if (!module->post_init) {
            continue;
        }
        int ret = module->post_init(context, module->state);
        errors += !!(ret != 0);
        ICP_LOG(ICP_LOG_INFO, "Post-initializing %s module: %s\n",
                module->info.id, ret ? "Failed" : "OK");
    }

    return (errors);
}

int icp_modules_start()
{
    struct icp_module *module = NULL;
    int errors = 0;

    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        if (!module->start) {
            continue;
        }
        int ret = module->start(module->state);
        errors += !!(ret != 0);
        ICP_LOG(ICP_LOG_INFO, "Starting %s module: %s\n",
                module->info.id, ret ? "Failed" : "OK");
    }

    return (errors);
}

void icp_modules_finish()
{
    struct icp_module *module = NULL;
    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        if (!module->finish) {
            continue;
        }
        module->finish(module->state);
    }
}

int icp_get_loaded_module_count()
{
    int loaded_module_count = 0;
    struct icp_module * module = NULL;

    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        loaded_module_count++;
    }

    return loaded_module_count;
}

const struct icp_module_info * icp_get_module_info_by_id(const char * module_id)
{
    if(!module_id)
        return NULL;

    struct icp_module *module = NULL;
    SLIST_FOREACH(module, &icp_modules_list_head, next) {

        if (strncmp(module_id, module->info.id, ICP_MAX_MODULE_ID_LENGTH) == 0) {
            return &module->info;
        }
    }

    return NULL;
}

int icp_get_module_info_list(const struct icp_module_info * info[], int max_entries)
{
    if (!info)
        return -1;

    if (max_entries < 0)
        return -1;
    else if (max_entries == 0)
        return icp_get_loaded_module_count();

    int module_list_count = 0;
    struct icp_module *module = NULL;
    SLIST_FOREACH(module, &icp_modules_list_head, next) {
        info[module_list_count] = &module->info;
        module_list_count++;

        if (module_list_count == max_entries)
            return module_list_count;
    }

    return module_list_count;
}
