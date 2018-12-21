#include <assert.h>
#include <stddef.h>

#include "core/icp_log.h"
#include "core/icp_modules.h"

static SLIST_HEAD(icp_modules_list, icp_module) icp_modules_list_head =
    SLIST_HEAD_INITIALIZER(icp_modules_list_head);

void icp_modules_register(struct icp_module *module)
{
    assert(module->name);  /* try to catch init order problems */
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
                module->name, ret ? "Failed" : "OK");
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
                module->name, ret ? "Failed" : "OK");
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
                module->name, ret ? "Failed" : "OK");
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
                module->name, ret ? "Failed" : "OK");
    }

    return (errors);
}
