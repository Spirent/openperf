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
 * Structure describing details of a module
 */
#define ICP_MAX_MODULE_ID_LENGTH 80
#define ICP_MODULE_ID_REGEX "^[a-zA-Z0-9\\-_.]+$"
struct icp_module_info {
    const char *id;
    const char *description;

    struct version {
        int version;
        const char *build_number;
        const char *build_date;
        const char *source_commit;
    } version;

    const char *linkage;
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
int icp_get_loaded_module_count();

/**
 * Find a loaded module by id (name).
 *
 * @param module_id
 *   Null-terminated string representing module id to search for.
 *
 * @return
 *   if located, module information struct otherwise NULL
 */
const struct icp_module_info * icp_get_module_info_by_id(const char * module_id);

/**
 * Get a list of module information structs for all modules currently
 * loaded.
 *
 * @param info
 *   pointer to an array of const pointers to module info structs
 * @param max_entries
 *   maximum number of entries in the array pointed to by info
 *
 * @return
 *   on success return the actual number of modules info now points to
 *   otherwise -1
 */
int icp_get_module_info_list(const struct icp_module_info * info[], int max_entries);

/**
 * Workaround for preprocessor not exactly understanding initializer lists.
 * Otherwise the preprocessor interprets the initializer list as multiple
 * arguments to the REGISTER_MODULE macro itself.
 */

#define INIT_MODULE_INFO(id_, description_, version_,                   \
                         build_number_, build_date_, source_commit_,    \
                         linkage_, path_)                               \
    { .id = id_,                                                        \
      .description = description_,                                      \
      .version = {                                                      \
            .version = version_,                                        \
            .build_number = build_number_,                              \
            .build_date = build_date_,                                  \
            .source_commit = source_commit_                             \
       },                                                               \
       .linkage = linkage_,                                             \
       .path = path_                                                    \
    }                                                                   \

/**
 * Macro hackery for ensuring that the module struct is initialized *before*
 * we insert it into our list of modules.  If the struct is initialized
 * after it is inserted, then the next ptr will be changed back to NULL,
 * which would obviously cause items to be dropped from the list.
 * This is mainly a problem when state has a constructor of some sort.
 */
#define REGISTER_MODULE(m, info_, state_,                        \
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
