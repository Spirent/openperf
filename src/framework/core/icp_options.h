#ifndef _ICP_OPTIONS_H_
#define _ICP_OPTIONS_H_

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/queue.h>

/**
 * Signature for option based init function
 */
typedef int (icp_option_init_fn)();

/**
 * Signature for option handler callback
 */
typedef int (icp_option_callback_fn)(int opt, const char *optarg);

/**
 * A structure describing an option based init function
 */
struct icp_option {
    const char *description;
    const char *long_opt;
    int short_opt;
    int has_arg;
};

struct icp_options_data {
    SLIST_ENTRY(icp_options_data) next;
    const char *name;
    icp_option_init_fn *init;
    icp_option_callback_fn *callback;
    struct icp_option options[];
};

/**
 * Register an option init function
 *
 * @param[in] opt_init
 *   Pointer to an icp_option_init struct
 */
void icp_options_register(struct icp_options_data *opt_data);

/**
 * Call all registered option init functions
 */
int icp_options_init();

/**
 * Parse all options
 */
int icp_options_parse(int argc, char *argv[]);

/**
 * Hash a long option string to the size of a short one (int).
 */
int icp_options_hash_long(const char * long_opt);

/**
 * Macro for registring option handlers
 */
#define REGISTER_OPTIONS(o)                                             \
    void icp_options_register_ ## o(void);                              \
    void __attribute__((constructor)) icp_options_register_ ## o(void)  \
    {                                                                   \
        icp_options_register((struct icp_options_data *)&o);            \
    }

#ifdef __cplusplus
}
#endif

#endif /* _ICP_OPTIONS_H_ */
