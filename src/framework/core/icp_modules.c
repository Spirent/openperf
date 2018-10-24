#include <stddef.h>

#include "core/icp_log.h"
#include "core/icp_modules.h"

static struct icp_modules_list _modules_list =
    STAILQ_HEAD_INITIALIZER(_modules_list);

void icp_modules_register(struct icp_module *module)
{
    STAILQ_INSERT_TAIL(&_modules_list, module, next);
}

int icp_modules_pre_init(void *context)
{
    struct icp_module *module = NULL;
    int errors = 0;

    STAILQ_FOREACH(module, &_modules_list, next) {
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

    STAILQ_FOREACH(module, &_modules_list, next) {
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

    STAILQ_FOREACH(module, &_modules_list, next) {
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

    STAILQ_FOREACH(module, &_modules_list, next) {
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
