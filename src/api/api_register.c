#include "core/icp_core.h"

extern int api_option_handler(int opt, const char *opt_arg, void *opt_data);

struct icp_options_data api_options = {
    .name = "API",
    .init = NULL,
    .callback = api_option_handler,
    .data = NULL,
    .options = {
        {"API service port", "api-port", 'p', true},
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(api_options)

extern int api_service_init(void *context);

struct icp_module api_service = {
    .name = "API service",
    .init = api_service_init,
};

REGISTER_MODULE(api_service)
