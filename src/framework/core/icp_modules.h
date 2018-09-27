#ifndef _ICP_MODULES_H_
#define _ICP_MODULES_H_

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/queue.h>

STAILQ_HEAD(icp_modules_list, icp_module);

/**
 * Signature for module initialization function
 */
typedef int (icp_module_init_fn)(void *context, void *state);

typedef int (icp_module_start_fn)(void *state);

/**
 * Structure describing a module to initialize
 */
struct icp_module {
    STAILQ_ENTRY(icp_module) next;
    const char *name;
    void *state;
    icp_module_init_fn *pre_init;
    icp_module_init_fn *init;
    icp_module_init_fn *post_init;
    icp_module_start_fn *start;
};

/**
 * Register a module
 *
 * @param[in] module
 *   Pointer to an icp_module structure describing the module to register
 */
void icp_modules_register(struct icp_module *module);

/**
 * Invoke pre init callback for all registered modules
 *
 * @param[in] context
 *   ZeroMQ context for message passing
 *
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int icp_modules_pre_init(void *context);

/**
 * Invoke init callback for all registered modules
 *
 * @param[in] context
 *   ZeroMQ context for message passing
 *
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int icp_modules_init(void *context);

/**
 * Invoke post init callback for all registered modules
 *
 * @param[in] context
 *   ZeroMQ context for message passing
 *
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int icp_modules_post_init(void *context);

/**
 * Invoke start callback for all registered modules
 *
 * @param[in] context
 *   ZeroMQ context for message passing
 *
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int icp_modules_start();

#define REGISTER_MODULE(m)                                              \
    void icp_modules_register_ ## m(void);                              \
    void __attribute__((constructor)) icp_modules_register_ ## m(void)  \
    {                                                                   \
        icp_modules_register(&m);                                       \
    }

#ifdef __cplusplus
}
#endif

#endif /* _ICP_MODULES_H_ */
