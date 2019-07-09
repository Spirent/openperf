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
 * Enum denoting the type of an option's data.
 * Exact types are mapped as:
 * STRING -> std::string
 * LONG -> long
 * DOUBLE -> double
 * MAP -> std::map<std::string, std::string>>
 * LIST -> std::vector<std::string>
 *
 * NONE -> bool
 * The NONE -> bool thing means that if the option is present
 * framework will return true when queried for it, false otherwise.
 */
typedef enum icp_option_type {
    ICP_OPTION_TYPE_NONE = 0,
    ICP_OPTION_TYPE_STRING,
    ICP_OPTION_TYPE_LONG,
    ICP_OPTION_TYPE_DOUBLE,
    ICP_OPTION_TYPE_MAP,
    ICP_OPTION_TYPE_LIST
} icp_option_type_t;

/**
 * A structure describing an option based init function
 */
struct icp_option {
    const char *description;
    const char *long_opt;
    int short_opt;
    enum icp_option_type opt_type;
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
 * Retrieve long option string.
 * @param[in] op short-form option.
 *
 * @return
 *  pointer to a character string for the long form option,
 *  NULL otherwise.
 */
const char * icp_options_get_long_opt(int op);

/**
 * Retrieve type of an option value.
 * @param[in] op short-form option.
 *
 * @return
 *  value type for the given option, ICP_OPTION_TYPE_NONE if none can be found.
 */
enum icp_option_type icp_options_get_opt_type_short(int op);

/**
 * Retrieve type of an option value.
 * @param[in] long_opt long-form option.
 * @param[in] len length of long-form option.
 *
 * @return
 *  value type for the given option, ICP_OPTION_TYPE_NONE if none can be found.
 */
enum icp_option_type icp_options_get_opt_type_long(const char *long_opt, size_t len);

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
