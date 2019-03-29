#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "core/icp_log.h"
#include "core/icp_options.h"

static SLIST_HEAD(icp_options_data_head, icp_options_data) icp_options_data_head =
    SLIST_HEAD_INITIALIZER(icp_options_data_head);

static void __attribute__((noreturn)) _usage(const char *program_name)
{
    FILE *output = stderr;

    fprintf(output, "Usage: %s\n\n", program_name);
    fprintf(output, "Options:\n");

    struct icp_options_data *opt_data = NULL;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        for (const struct icp_option *curr = opt_data->options;
             curr->description != NULL;
             curr++) {
            if (curr->short_opt == 0) {
                fprintf(output, "      --%-22s%s\n",
                        curr->long_opt, curr->description);
            }
            else {
                fprintf(output, "  -%c, --%-22s%s\n",
                        curr->short_opt, curr->long_opt, curr->description);
            }
        }
    }

    exit(EXIT_SUCCESS);
}

int icp_options_init()
{
    struct icp_options_data *opt_data = NULL;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        if (opt_data->init) {
            if (opt_data->init() != 0) {
                ICP_LOG(ICP_LOG_ERROR, "Failed to initialize %s options\n", opt_data->name);
            }
        }
    }

    return (0);
}

struct icp_options_data * _find_options_data(int opt)
{
    struct icp_options_data *opt_data = NULL;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        for (const struct icp_option *curr = opt_data->options;
             curr->description != NULL;
             curr++) {
            if (curr->short_opt == opt) {
                return (opt_data);
            }
            else if (icp_options_hash_long(curr->long_opt) == opt) {
                return (opt_data);
            }
        }
    }
    return (NULL);
}

int _allocate_optstring(char **optstringp)
{
    /* Figure out necessary string length */
    int length = 1;  /* include trailing null */
    struct icp_options_data *opt_data = NULL;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        for (const struct icp_option *curr = opt_data->options;
             curr->description != NULL;
             curr++) {
            if (curr->short_opt) {
                length += curr->has_arg ? 2 : 1;
            }
        }
    }

    /* Allocate sufficient length */
    char *optstring = calloc(1, length);
    if (!optstring) {
        return (-ENOMEM);
    }

    /* Fill in string */
    unsigned idx = 0;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        for (const struct icp_option *curr = opt_data->options;
             curr->description != NULL;
             curr++) {
            if (curr->short_opt) {
                optstring[idx++] = curr->short_opt;
                if (curr->has_arg) {
                    optstring[idx++] = ':';
                }
            }
        }
    }

    *optstringp = optstring;
    return (0);
}

int _allocate_longopts(struct option **longoptsp)
{
    /* Figure out necessary longopts length */
    int nb_opts = 1;  /* include null terminated ending */
    struct icp_options_data *opt_data = NULL;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        for (const struct icp_option *curr = opt_data->options;
             curr->description != NULL;
             curr++) {
            nb_opts++;
        }
    }

    struct option *longopts = calloc(nb_opts, sizeof(*longopts));
    if (!longopts) {
        return (-ENOMEM);
    }

    /* Fill in longopts */
    unsigned idx = 0;
    SLIST_FOREACH(opt_data, &icp_options_data_head, next) {
        for (const struct icp_option *curr = opt_data->options;
             curr->description != NULL;
             curr++) {
            longopts[idx++] = (struct option){
                curr->long_opt,
                curr->has_arg,
                NULL,
                curr->short_opt ? curr->short_opt : icp_options_hash_long(curr->long_opt)
            };
        }
    }

    *longoptsp = longopts;
    return (0);
}

int icp_options_parse(int argc, char *argv[])
{
    optind = 1;  /* magic variable for getopt */
    opterr = 1;

    char *optstring = NULL;
    int error = _allocate_optstring(&optstring);
    if (error) {
        return (error);
    }

    struct option *longopts = NULL;
    if ((error = _allocate_longopts(&longopts)) != 0) {
        free(optstring);
        return (error);
    }

    for (;;) {
        int option_index = 0;
        int opt = getopt_long(argc, argv, optstring, longopts, &option_index);
        if (opt == -1) {
            break;
        }
        if (opt == '?' || opt == 'h') {
            /* unspecified option */
            _usage(argv[0]);
        }
        struct icp_options_data *opt_data = _find_options_data(opt);
        assert(opt_data);
        if (opt_data->callback) {
            opt_data->callback(opt, optarg);
        }
    }

    free(optstring);
    free(longopts);
    return (0);
}

/*
 * Hash a long option string to the size of a short one (int).
 * This assumes long_opt points to a NULL-terminated string.
 * Using FNV-1a for simplicity.
 * https://tools.ietf.org/html/draft-eastlake-fnv-03
 */
int icp_options_hash_long(const char * long_opt)
{
    /* Initialize hash with the 32-bit offset_basis */
    unsigned int hash = 2166136261;
    static const unsigned int fnv_prime = 16777619;
    const char * val = long_opt;

    while (*val != '\0') {
        hash ^= *val++;
        hash *= fnv_prime;
    }

    return hash;
}

void icp_options_register(struct icp_options_data *opt_data)
{
    assert(opt_data->name);  /* catch init ordering problems */
    SLIST_INSERT_HEAD(&icp_options_data_head, opt_data, next);
}

/*
 * Register the help option.
 * Note: we don't register any callbacks because we can handle them directly.
 * Additionally, there is no mechanism to get argv, which we need for calling
 * usage.
 */
static struct icp_options_data help_option = {
    .name = "help",
    .init = NULL,
    .callback = NULL,
    .options = {
        { "Print this message.", "help", 'h', 0 },
        { 0, 0, 0, 0 },
    }
};

REGISTER_OPTIONS(help_option)
