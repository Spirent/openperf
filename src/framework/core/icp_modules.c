#include <stddef.h>

#include "core/icp_log.h"
#include "core/icp_modules.h"

static struct icp_modules_list _modules_list =
    STAILQ_HEAD_INITIALIZER(_modules_list);

void icp_modules_register(struct icp_module *module)
{
    STAILQ_INSERT_TAIL(&_modules_list, module, next);
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
        icp_log(ICP_LOG_INFO, "Initializing %s module: %s\n",
                module->name, ret ? "Failed" : "OK");
    }

    return (errors);
}

int icp_modules_start(void *context)
{
    struct icp_module *module = NULL;
    int errors = 0;

    STAILQ_FOREACH(module, &_modules_list, next) {
        if (!module->start) {
            continue;
        }
        int ret = module->start(context, module->state);
        errors += !!(ret != 0);
        icp_log(ICP_LOG_INFO, "Starting %s module: %s\n",
                module->name, ret ? "Failed" : "OK");
    }

    return (errors);
}
