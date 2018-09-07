#include <stddef.h>

#include "core/icp_log.h"
#include "core/icp_modules.h"

static struct icp_modules_list _modules_list =
    TAILQ_HEAD_INITIALIZER(_modules_list);

void icp_modules_register(struct icp_module *module)
{
    TAILQ_INSERT_TAIL(&_modules_list, module, next);
}

int icp_modules_init(void *context)
{
    struct icp_module *module = NULL;
    int errors = 0;

    TAILQ_FOREACH(module, &_modules_list, next) {
        int ret = module->init(context);
        errors += !!(ret != 0);
        icp_log(ICP_LOG_INFO, "Initializing module %s: %s\n",
                module->name, ret ? "Failed" : "OK");
    }

    return (errors);
}
