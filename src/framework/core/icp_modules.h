#ifndef _ICP_MODULES_H_
#define _ICP_MODULES_H_

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/queue.h>
#include <stddef.h>

/**
 * Signature for module init/start/stop functions
 */
typedef int (icp_module_init_fn)(void *context, void *state);

typedef void (icp_module_fini_fn)(void *state);

typedef int (icp_module_start_fn)(void *state);

/**
 * Structure describing details of a module
 */
#define ICP_MODULE_ID_REGEX "^[a-z0-9.\\-]+$"
enum icp_module_linkage_type { NONE, DYNAMIC, STATIC, MAX };
struct icp_module_info {
    const char *id;
    const char *description;

    struct version {
        int version;
        const char *build_number;
        const char *build_date;
        const char *source_commit;
    } version;

    enum icp_module_linkage_type linkage;
    const char *path;
};

/**
 * Structure describing a module to initialize
 */
struct icp_module {
    SLIST_ENTRY(icp_module) next;
    struct icp_module_info info;
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
 * Get a count of how many modules are currently loaded.
 *
 * @return
 *   Number of currently loaded modules.
 */
size_t icp_modules_get_loaded_count();

/**
 * Find a loaded module by id (name).
 *
 * @param module_id
 *   Null-terminated string representing module id to search for.
 *
 * @return
 *   if located, module information struct otherwise NULL
 */
const struct icp_module_info * icp_modules_get_info_by_id(const char * module_id);

/**
 * Get a list of module information structs for all modules currently
 * loaded.
 *
 * @param info
 *   pointer to an array of const pointers to module info structs
 * @param max_entries
 *   maximum number of entries in the array pointed to by info.
 *   a value of 0 will return the current loaded module count. info[] would not be modified.
 *
 * @return
 *   on success return the actual number of modules info now points to
 *   otherwise -1. except if max_entries is 0, then return current loaded module count.
 */
int icp_modules_get_info_list(const struct icp_module_info * info[], size_t max_entries);

/**
 * Workaround for preprocessor not exactly understanding initializer lists.
 * Otherwise the preprocessor interprets the initializer list as multiple
 * arguments to the REGISTER_MODULE macro itself.
 */

#define INIT_MODULE_INFO(id_, description_, version_)                   \
    { .id = id_,                                                        \
      .description = description_,                                      \
      .version = {                                                      \
            .version = version_,                                        \
            .build_number = BUILD_NUMBER,                               \
            .build_date = BUILD_TIMESTAMP,                              \
            .source_commit = BUILD_COMMIT                               \
       },                                                               \
       .linkage = STATIC,                                               \
       .path = NULL                                                     \
    }                                                                   \

/**
 * Macro hackery for ensuring that the module struct is initialized *before*
 * we insert it into our list of modules.  If the struct is initialized
 * after it is inserted, then the next ptr will be changed back to NULL,
 * which would obviously cause items to be dropped from the list.
 * This is mainly a problem when state has a constructor of some sort.
 */
#define REGISTER_MODULE(m, info_, state_,                               \
                        pre_init_, init_, post_init_,                   \
                        start_, finish_)                                \
    static struct icp_module m;                                         \
    __attribute__((constructor (100)))                                  \
    void icp_modules_init_ ## m(void)                                   \
    {                                                                   \
        m = { .info = info_,                                            \
              .state = state_,                                          \
              .pre_init = pre_init_,                                    \
              .init = init_,                                            \
              .post_init = post_init_,                                  \
              .start = start_,                                          \
              .finish = finish_                                         \
        };                                                              \
    }                                                                   \
    __attribute__((constructor (200)))                                  \
    void icp_modules_register_ ## m(void)                               \
    {                                                                   \
        icp_modules_register(&m);                                       \
    }

#ifdef __cplusplus
}
#endif

#endif /* _ICP_MODULES_H_ */
