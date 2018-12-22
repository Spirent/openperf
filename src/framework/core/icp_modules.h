#ifndef _ICP_MODULES_H_
#define _ICP_MODULES_H_

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/queue.h>

/**
 * Signature for module init/start/stop functions
 */
typedef int (icp_module_init_fn)(void *context, void *state);

typedef void (icp_module_fini_fn)(void *state);

typedef int (icp_module_start_fn)(void *state);

/**
 * Structure describing a module to initialize
 */
struct icp_module {
    SLIST_ENTRY(icp_module) next;
    const char *name;
    void *state;
    icp_module_init_fn  *pre_init;
    icp_module_init_fn  *init;
    icp_module_init_fn  *post_init;
    icp_module_start_fn *start;
    icp_module_fini_fn  *finish;
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


/**
 * Invoke finish callback for all registered modules
 */
void icp_modules_finish();

/**
 * Macro hackery for ensuring that the module struct is initialized *before*
 * we insert it into our list of modules.  If the struct is initialized
 * after it is inserted, then the next ptr will be changed back to NULL,
 * which would obviously cause items to be dropped from the list.
 * This is mainly a problem when state has a constructor of some sort.
 */
#define REGISTER_MODULE(m, name_, state_,                               \
                        pre_init_, init_, post_init_,                   \
                        start_, finish_)                                \
    static struct icp_module m;                                         \
    __attribute__((constructor (100)))                                  \
    void icp_modules_init_ ## m(void)                                   \
    {                                                                   \
        m = { .name = name_,                                            \
              .state = state_,                                          \
              .pre_init = pre_init_,                                    \
              .init = init_,                                            \
              .post_init = post_init_,                                  \
              .start = start_,                                          \
              .finish = finish_                                         \
        };                                                              \
    }                                                                   \
    __attribute__((constructor (200))) \
    void icp_modules_register_ ## m(void)                               \
    {                                                                   \
        icp_modules_register(&m);                                       \
    }

#ifdef __cplusplus
}
#endif

#endif /* _ICP_MODULES_H_ */
