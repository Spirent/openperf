#ifndef _OP_OPTIONS_H_
#define _OP_OPTIONS_H_

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
typedef int (op_option_init_fn)();

/**
 * Signature for option handler callback
 */
typedef int (op_option_callback_fn)(int opt, const char *optarg);

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
typedef enum op_option_type {
    OP_OPTION_TYPE_NONE = 0,
    OP_OPTION_TYPE_STRING,
    OP_OPTION_TYPE_LONG,
    OP_OPTION_TYPE_DOUBLE,
    OP_OPTION_TYPE_MAP,
    OP_OPTION_TYPE_LIST
} op_option_type_t;

/**
 * A structure describing an option based init function
 */
struct op_option {
    const char *description;
    const char *long_opt;
    int short_opt;
    enum op_option_type opt_type;
};

struct op_options_data {
    SLIST_ENTRY(op_options_data) next;
    const char *name;
    op_option_init_fn *init;
    op_option_callback_fn *callback;
    struct op_option options[];
};

/**
 * Register an option init function
 *
 * @param[in] opt_init
 *   Pointer to an op_option_init struct
 */
void op_options_register(struct op_options_data *opt_data);

/**
 * Call all registered option init functions
 */
int op_options_init();

/**
 * Parse all options
 */
int op_options_parse(int argc, char *argv[]);

/**
 * Hash a long option string to the size of a short one (int).
 */
int op_options_hash_long(const char * long_opt);

/**
 * Retrieve long option string.
 * @param[in] op short-form option.
 *
 * @return
 *  pointer to a character string for the long form option,
 *  NULL otherwise.
 */
const char * op_options_get_long_opt(int op);

/**
 * Retrieve type of an option value.
 * @param[in] op short-form option.
 *
 * @return
 *  value type for the given option, OP_OPTION_TYPE_NONE if none can be found.
 */
enum op_option_type op_options_get_opt_type_short(int op);

/**
 * Retrieve type of an option value.
 * @param[in] long_opt long-form option.
 * @param[in] len length of long-form option.
 *
 * @return
 *  value type for the given option, OP_OPTION_TYPE_NONE if none can be found.
 */
enum op_option_type op_options_get_opt_type_long(const char *long_opt, size_t len);

/**
 * Macro for registring option handlers
 */
#define REGISTER_OPTIONS(o)                                             \
    void op_options_register_ ## o(void);                              \
    void __attribute__((constructor)) op_options_register_ ## o(void)  \
    {                                                                   \
        op_options_register((struct op_options_data *)&o);            \
    }

#ifdef __cplusplus
}
#endif

#endif /* _OP_OPTIONS_H_ */
