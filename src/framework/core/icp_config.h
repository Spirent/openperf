#ifndef _ICP_CONFIG_H_
#define _ICP_CONFIG_H_

/**
 * @file
 */

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ICP_CONFIG_NAME_LEN 32
#define ICP_CONFIG_MAX_STRING_LEN (size_t)PATH_MAX

/**
 * Specifies the type of the associated data.
 */
enum icp_config_type {
    CONFIG_TYPE_NONE = 0,  /**< No type information */
    CONFIG_TYPE_BOOL,      /**< boolean value */
    CONFIG_TYPE_DOUBLE,    /**< double value */
    CONFIG_TYPE_UINT32,    /**< unsigned 32 bit int */
    CONFIG_TYPE_UINT64,    /**< unsigned 64 bit int */
    CONFIG_TYPE_STRING,    /**< Null terminated string */
    CONFIG_TYPE_BINARY,    /**< binary blob */
};

union icp_config_value {
    bool     value_bool;
    double   value_double;
    uint32_t value_uint32;
    uint64_t value_uint64;
    char    *value_string;
    uint8_t *value_binary;
};

/**
 * Structure for getting and setting configuration tree values
 * via icp_config_{get,set}_value.
 */
struct icp_config_data  {
    union icp_config_value value;  /**< The value of this data */
    size_t length;                 /**< The size of this data */
    struct {
        uint64_t msg_id;           /**< ID of message providing this data */
        int msg_fd;                /**< FD of socket providing this data */
    } meta;
    enum icp_config_type type;     /**< The type of this data */
};

/**
 * Static initialization macros
 */
#define ICP_CONFIG_DATA_INIT { { 0 }, 0, { UINT64_MAX, -1 }, CONFIG_TYPE_NONE }

#define icp_config_data_bool_init(x)                                    \
    { { .value_bool = x }, sizeof(bool), { UINT64_MAX, -1 }, CONFIG_TYPE_BOOL }
#define icp_config_data_double_init(x)                                  \
    { { .value_double = x }, sizeof(double), { UINT64_MAX, -1 }, CONFIG_TYPE_DOUBLE }
#define icp_config_data_uint32_init(x)                                  \
    { { .value_uint32 = x }, sizeof(uint32_t), { UINT64_MAX, -1 }, CONFIG_TYPE_UINT32 }
#define icp_config_data_uint64_init(x)                                  \
    { { .value_uint64 = x }, sizeof(uint64_t), { UINT64_MAX, -1 }, CONFIG_TYPE_UINT64 }
#define icp_config_data_string_init(x)                                  \
    { { .value_string = x }, sizeof(x), { UINT64_MAX, -1 }, CONFIG_TYPE_STRING }

/*
 * The next section has a bunch of C11 _Generic macros to facilitate
 * type safe getting/setting of values with our generic data structure
 */

/**
 * Macro to get the configuration type from any supported data type
 */
#define get_icp_config_type(x)                                          \
    _Generic((x),                                                       \
             bool: CONFIG_TYPE_BOOL,                                    \
             const bool: CONFIG_TYPE_BOOL,                              \
             double: CONFIG_TYPE_DOUBLE,                                \
             const double: CONFIG_TYPE_DOUBLE,                          \
             uint32_t: CONFIG_TYPE_UINT32,                              \
             const uint32_t: CONFIG_TYPE_UINT32,                        \
             uint64_t: CONFIG_TYPE_UINT64,                              \
             const uint64_t: CONFIG_TYPE_UINT64,                        \
             char *: CONFIG_TYPE_STRING,                                \
             char[sizeof(x)]: CONFIG_TYPE_STRING,                       \
             const char *: CONFIG_TYPE_STRING,                          \
             const char[sizeof(x)]: CONFIG_TYPE_STRING,                 \
             default: CONFIG_TYPE_BINARY                                \
        )

/**
 * Macros to use for setting data in the icp_config_value union
 *
 * Note: the config tree will makes copies of string/binary data, so
 * while unpleasant, casting away the constness is safe.
 */
#define VALUE_SET_FN(u_name, c_type)                                    \
    static inline void                                                  \
    _set_icp_config_value_##u_name(union icp_config_value *value,       \
                                   const c_type x)                      \
    {                                                                   \
        value->value_##u_name = (c_type)x;                              \
    }

VALUE_SET_FN(bool, bool)
VALUE_SET_FN(double, double)
VALUE_SET_FN(uint32, uint32_t)
VALUE_SET_FN(uint64, uint64_t)
VALUE_SET_FN(string, char *)
VALUE_SET_FN(binary, uint8_t *)

#undef VALUE_SET_FN

#define set_icp_config_value(value, x)                                  \
    _Generic((x),                                                       \
             bool: _set_icp_config_value_bool,                          \
             const bool: _set_icp_config_value_bool,                    \
             double: _set_icp_config_value_double,                      \
             const double: _set_icp_config_value_double,                \
             uint32_t: _set_icp_config_value_uint32,                    \
             const uint32_t: _set_icp_config_value_uint32,              \
             uint64_t: _set_icp_config_value_uint64,                    \
             const uint64_t: _set_icp_config_value_uint64,              \
             char *: _set_icp_config_value_string,                      \
             const char *: _set_icp_config_value_string,                \
             char[sizeof(x)]: _set_icp_config_value_string,             \
             const char[sizeof(x)]: _set_icp_config_value_string,       \
             default: _set_icp_config_value_binary                      \
        )(value, x)

/**
 * Macros to use for getting data from the icp_config_value union
 * in the correct type
 */
#define VALUE_GET_FN(u_name, c_type)                                    \
    static inline void                                                  \
    _get_icp_config_value_##u_name(const union icp_config_value *value, \
                                   c_type *x)                           \
    {                                                                   \
        *x = value->value_##u_name;                                     \
    }

VALUE_GET_FN(bool, bool)
VALUE_GET_FN(double, double)
VALUE_GET_FN(uint32, uint32_t)
VALUE_GET_FN(uint64, uint64_t)
VALUE_GET_FN(string, char *)
VALUE_GET_FN(binary, uint8_t *)

#undef VALUE_GET_FN

#define get_icp_config_value(value, x)                                  \
    _Generic((x),                                                       \
             bool *: _get_icp_config_value_bool,                        \
             double *: _get_icp_config_value_double,                    \
             uint32_t *: _get_icp_config_value_uint32,                  \
             uint64_t *: _get_icp_config_value_uint64,                  \
             char **: _get_icp_config_value_string,                     \
             char *[sizeof(x)]: _get_icp_config_value_string,           \
             uint8_t **: _get_icp_config_value_binary,                  \
             uint8_t *[sizeof(x)]: _get_icp_config_value_binary         \
        )(value, x)

#define POPULATE_DATA_FN(c_name, c_type)                            \
    static inline void                                              \
    icp_config_populate_data_##c_name(struct icp_config_data *data, \
                                      const c_type value,           \
                                      size_t length)                \
    {                                                               \
        assert(data);                                               \
        data->type = get_icp_config_type(value);                    \
        data->length = length;                                      \
        set_icp_config_value(&data->value, value);                  \
        data->meta.msg_id = UINT64_MAX;                             \
        data->meta.msg_fd = -1;                                     \
    }

POPULATE_DATA_FN(bool, bool)
POPULATE_DATA_FN(double, double)
POPULATE_DATA_FN(uint32, uint32_t)
POPULATE_DATA_FN(uint64, uint64_t)

#undef POPULATE_DATA_FN

static inline void
icp_config_populate_data_string(struct icp_config_data *data,
                                const char * value,
                                size_t length __attribute__((unused)))
{
    assert(data);
    assert(value);
    data->type = CONFIG_TYPE_STRING;
    data->length = strlen(value);
    set_icp_config_value(&data->value, value);
    data->meta.msg_id = UINT64_MAX;
    data->meta.msg_fd = -1;
}

/**
 * A macro for populating a default icp_config_data structure
 *
 * @param[in] data
 *    The icp_config_data structure to populate
 * @param[in] value
 *    The value to put into data
 */
#define icp_config_populate_data(data, value)                           \
    _Generic((value),                                                   \
             bool: icp_config_populate_data_bool,                       \
             const bool: icp_config_populate_data_bool,                 \
             double: icp_config_populate_data_double,                   \
             const double: icp_config_populate_data_double,             \
             uint32_t: icp_config_populate_data_uint32,                 \
             const uint32_t: icp_config_populate_data_uint32,           \
             uint64_t: icp_config_populate_data_uint64,                 \
             const uint64_t: icp_config_populate_data_uint64,           \
             char *: icp_config_populate_data_string,                   \
             const char *: icp_config_populate_data_string,             \
             char[sizeof(value)]: icp_config_populate_data_string,      \
             const char[sizeof(value)]: icp_config_populate_data_string \
        )(data, value, sizeof(value))

#define POPULATE_DATA_META_FN(c_name, c_type)                           \
    static inline void                                                  \
    icp_config_populate_data_meta_##c_name(struct icp_config_data *data, \
                                           const c_type value,          \
                                           size_t length,               \
                                           uint64_t id,                 \
                                           int fd)                      \
    {                                                                   \
        assert(data);                                                   \
        data->type = get_icp_config_type(value);                        \
        data->length = length;                                          \
        set_icp_config_value(&data->value, value);                      \
        data->meta.msg_id = id;                                         \
        data->meta.msg_fd = fd;                                         \
    }

POPULATE_DATA_META_FN(bool, bool)
POPULATE_DATA_META_FN(double, double)
POPULATE_DATA_META_FN(uint32, uint32_t)
POPULATE_DATA_META_FN(uint64, uint64_t)

#undef POPULATE_DATA_META_FN

static inline void
icp_config_populate_data_meta_string(struct icp_config_data *data,
                                     const char * value,
                                     size_t length __attribute__((unused)),
                                     uint64_t id,
                                     int fd)
{
    assert(data);
    assert(value);
    data->type = CONFIG_TYPE_STRING;
    data->length = strlen(value);
    set_icp_config_value(&data->value, value);
    data->meta.msg_id = id;
    data->meta.msg_fd = fd;
}

/**
 * A macro for populating a meta icp_config_data structure
 *
 * @param[in] data
 *    The icp_config_data structure to populate
 * @param[in] value
 *    The value to put into data
 * @param[in] id
 *    The originating message id
 * @param[in] fd
 *    The originaing socket fd
 */
#define icp_config_populate_data_meta(data, value, id, fd)              \
    _Generic((value),                                                   \
             bool: icp_config_populate_data_meta_bool,                  \
             const bool: icp_config_populate_data_meta_bool,            \
             double: icp_config_populate_data_meta_double,              \
             const double: icp_config_populate_data_meta_double,        \
             uint32_t: icp_config_populate_data_meta_uint32,            \
             const uint32_t: icp_config_populate_data_meta_uint32,      \
             uint64_t: icp_config_populate_data_meta_uint64,            \
             const uint64_t: icp_config_populate_data_meta_uint64,      \
             char *: icp_config_populate_data_meta_string,              \
             const char *: icp_config_populate_data_meta_string,        \
             char[sizeof(value)]: icp_config_populate_data_meta_string, \
             const char[sizeof(value)]: icp_config_populate_data_meta_string \
        )(data, value, sizeof(value), id, fd)

/**
 * Functions/macros for allocating fully formed and populated
 * default icp_config_data structures
 */
#define DATA_ALLOC_FN(u_name, c_type)                                   \
    static inline struct icp_config_data *                              \
    _icp_config_data_allocate_##u_name(c_type value) {                  \
        struct icp_config_data *data =                                  \
            (struct icp_config_data *)malloc(sizeof(*data));            \
        if (!data)                                                      \
            return (NULL);                                              \
        icp_config_populate_data(data, value);                          \
        return (data);                                                  \
    }

DATA_ALLOC_FN(bool, bool)
DATA_ALLOC_FN(double, double)
DATA_ALLOC_FN(uint32, uint32_t)
DATA_ALLOC_FN(uint64, uint64_t)

#undef DATA_ALLOC_FN

static inline struct icp_config_data *
_icp_config_data_allocate_string(const char *value);

#define DATA_ALLOC_COPY_FN(u_name, c_type)                              \
    static inline struct icp_config_data *                              \
    _icp_config_data_allocate_copy_##u_name(const c_type value,         \
                                            size_t length) {            \
        struct icp_config_data *data =                                  \
            (struct icp_config_data *)calloc(1, sizeof(*data));         \
        if (!data)                                                      \
            return (NULL);                                              \
        data->value.value_##u_name = (c_type)calloc(1, length);         \
        if (!data->value.value_##u_name) {                              \
            free(data);                                                 \
            return (NULL);                                              \
        }                                                               \
        memcpy(data->value.value_##u_name, value, length);              \
        data->length = length;                                          \
        data->type = get_icp_config_type(value);                        \
        return (data);                                                  \
    }

DATA_ALLOC_COPY_FN(string, char *)
DATA_ALLOC_COPY_FN(binary, uint8_t *)

#undef DATA_ALLOC_COPY_FN

#define icp_config_data_allocate1(x)                                    \
    _Generic((x),                                                       \
             bool: _icp_config_data_allocate_bool,                      \
             const bool: _icp_config_data_allocate_bool,                \
             double: _icp_config_data_allocate_double,                  \
             const double: _icp_config_data_allocate_double,            \
             uint32_t: _icp_config_data_allocate_uint32,                \
             const uint32_t: _icp_config_data_allocate_uint32,          \
             uint64_t: _icp_config_data_allocate_uint64,                \
             const uint64_t: _icp_config_data_allocate_uint64,          \
             char *: _icp_config_data_allocate_string,                  \
             const char *: _icp_config_data_allocate_string,            \
             char [sizeof(x)]: _icp_config_data_allocate_string,        \
             const char [sizeof(x)]: _icp_config_data_allocate_string   \
        )(x)

#define icp_config_data_allocate2(x, length)                            \
    _Generic((x),                                                       \
             default: _icp_config_data_allocate_copy_binary             \
        )(x, length)

static inline struct icp_config_data *
_icp_config_data_allocate_string(const char *value)
{
    return _icp_config_data_allocate_copy_string(value,
                                                 strlen(value) + 1);
}

/**
 * Functions/macros for allocating fully formed and populated
 * meta icp_config_data structures
 */
#define DATA_META_ALLOC_FN(u_name, c_type)                              \
    static inline struct icp_config_data *                              \
    _icp_config_data_meta_allocate_##u_name(c_type value,               \
                                            uint64_t id, int fd) {      \
        struct icp_config_data *data =                                  \
            (struct icp_config_data *)calloc(1, sizeof(*data));         \
        if (!data)                                                      \
            return (NULL);                                              \
        icp_config_populate_data_meta(data, value, id, fd);             \
        return (data);                                                  \
    }

DATA_META_ALLOC_FN(bool, bool)
DATA_META_ALLOC_FN(double, double)
DATA_META_ALLOC_FN(uint32, uint32_t)
DATA_META_ALLOC_FN(uint64, uint64_t)

#undef DATA_META_ALLOC_FN

static inline struct icp_config_data *
_icp_config_data_meta_allocate_string(const char *value,
                                      uint64_t id, int fd);

#define DATA_META_ALLOC_COPY_FN(u_name, c_type)                         \
    static inline struct icp_config_data *                              \
    _icp_config_data_meta_allocate_copy_##u_name(const c_type value,    \
                                                 size_t length,         \
                                                 uint64_t id, int fd) { \
        struct icp_config_data *data =                                  \
            (struct icp_config_data *)calloc(1, sizeof(*data));         \
        if (!data)                                                      \
            return (NULL);                                              \
        data->value.value_##u_name = (c_type)calloc(1, length);         \
        if (!data->value.value_##u_name) {                              \
            free(data);                                                 \
            return (NULL);                                              \
        }                                                               \
        data->type = get_icp_config_type(value);                        \
        data->length = length;                                          \
        memcpy(data->value.value_##u_name, value, length);              \
        data->meta.msg_id = id;                                         \
        data->meta.msg_fd = fd;                                         \
        return (data);                                                  \
    }

DATA_META_ALLOC_COPY_FN(string, char *)
DATA_META_ALLOC_COPY_FN(binary, uint8_t *)

#undef DATA_META_ALLOC_COPY_FN

#define icp_config_data_meta_allocate3(x, id, fd)                       \
    _Generic((x),                                                       \
             bool: _icp_config_data_meta_allocate_bool,                 \
             const bool: _icp_config_data_meta_allocate_bool,           \
             double: _icp_config_data_meta_allocate_double,             \
             const double: _icp_config_data_meta_allocate_double,       \
             uint32_t: _icp_config_data_meta_allocate_uint32,           \
             const uint32_t: _icp_config_data_meta_allocate_uint32,     \
             uint64_t: _icp_config_data_meta_allocate_uint64,           \
             const uint64_t: _icp_config_data_meta_allocate_uint64,     \
             char *: _icp_config_data_meta_allocate_string,             \
             const char *: _icp_config_data_meta_allocate_string,       \
             char [sizeof(x)]: _icp_config_data_meta_allocate_string,   \
             const char [sizeof(x)]: _icp_config_data_meta_allocate_string \
        )(x, id, fd)

#define icp_config_data_meta_allocate4(x, length, id, fd)               \
    _Generic((x),                                                       \
             default: _icp_config_data_meta_allocate_copy_binary        \
        )(x, length, id, fd)

static inline struct icp_config_data *
_icp_config_data_meta_allocate_string(const char *value, uint64_t id, int fd)
{
    return _icp_config_data_meta_allocate_copy_string(value,
                                                      strlen(value) + 1,
                                                      id, fd);
}

/* Syntactic sugar for allocating icp_config_data structures */
#define GET_DATA_ALLOCATE_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define icp_config_data_allocate(...)                               \
    GET_DATA_ALLOCATE_MACRO(__VA_ARGS__,                            \
                            icp_config_data_meta_allocate4,         \
                            icp_config_data_meta_allocate3,         \
                            icp_config_data_allocate2,              \
                            icp_config_data_allocate1)(__VA_ARGS__)

static inline void
icp_config_data_free(struct icp_config_data **datap) {
    struct icp_config_data *data = *datap;
    switch (data->type) {
    case CONFIG_TYPE_STRING:
        free(data->value.value_string);
        break;
    case CONFIG_TYPE_BINARY:
        free(data->value.value_binary);
        break;
    default:
        break;
    }
    free(data);
    *datap = NULL;
}

/**
 * Helpful debug functions
 */

static inline const char *icp_config_get_type_string(enum icp_config_type type)
{
    switch (type) {
    case CONFIG_TYPE_NONE: return "none";
    case CONFIG_TYPE_BOOL: return "bool";
    case CONFIG_TYPE_DOUBLE: return "double";
    case CONFIG_TYPE_UINT32: return "uint32_t";
    case CONFIG_TYPE_UINT64: return "uint64_t";
    case CONFIG_TYPE_STRING: return "char *";
    case CONFIG_TYPE_BINARY: return "uint8_t *";
    default: return "unknown";
    }
}

/* Print data on the command line (for debug use) */
void icp_config_data_dump(const struct icp_config_data *data);

#endif
