#ifndef _MEMORY_GENERATOR_H_
#define _MEMORY_GENERATOR_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory/op_packed_array.h"

extern const char memory_generator_commands[];

extern const char memory_generator_config_prefix[];
extern const char memory_generator_config_buffer_size[];
extern const char memory_generator_config_read_rate[];
extern const char memory_generator_config_read_size[];
extern const char memory_generator_config_read_threads[];
extern const char memory_generator_config_write_rate[];
extern const char memory_generator_config_write_size[];
extern const char memory_generator_config_write_threads[];
extern const char memory_generator_config_pattern[];
extern const char memory_generator_config_running[];
extern const char memory_generator_config_progress_meter[];
extern const char memory_generator_config_expected[];

extern const char memory_generator_tag_read[];
extern const char memory_generator_tag_write[];
#define MEMORY_GENERATOR_TAG_LENGTH 8

#define MEMORY_WORKER_IDX_MASK 0xfff
#define MEMORY_WORKER_IDX_SHIFT 4
#define MEMORY_WORKER_COUNT_MASK 0xfff
#define MEMORY_WORKER_COUNT_SHIFT 20

#define ICP_TASK_ARGS_NAME_MAX 16

/**
 * Structure describing the statistics collected from worker threads
 */
struct memory_generator_io_stats
{
    atomic_uint_fast64_t
        time_ns; /**< The number of ns required for the operations */
    atomic_uint_fast64_t operations; /**< The number of operations performed */
    atomic_uint_fast64_t bytes;      /**< The number of bytes read or written */
    atomic_uint_fast64_t
        errors; /**< The number of errors during reading or writing */
} __attribute__((aligned(64)));

enum memory_generator_msg_type {
    MEMORY_MSG_NONE = 0,
    MEMORY_MSG_CONFIG,
    MEMORY_MSG_LOAD,
    MEMORY_MSG_START,
    MEMORY_MSG_STOP,
    MEMORY_MSG_ABORT,
    MEMORY_MSG_MAX,
};

enum icp_generator_pattern {
    GENERATOR_PATTERN_NONE = 0,
    GENERATOR_PATTERN_RANDOM,
    GENERATOR_PATTERN_SEQUENTIAL,
    GENERATOR_PATTERN_REVERSE,
    GENERATOR_PATTERN_MAX,
};

/**
 * Structure describing the control message between the coordinator task and
 * workers
 */
struct memory_generator_control_msg
{
    char tag[MEMORY_GENERATOR_TAG_LENGTH];
    enum memory_generator_msg_type type;
    union
    {
        struct
        {
            struct
            {
                uint8_t* ptr;
                size_t size;
            } buffer;
            size_t io_size;
            enum icp_generator_pattern io_pattern;
            struct
            {
                void (*fn)(void*, uint64_t);
                void* arg;
            } callback;
        } config;
        struct
        {
            size_t rate;
        } load;
        const char* sync_endpoint;
    };
};

/**
 * This function manages memory load generation
 */
void* memory_generator_coordinator_task(void* args);

/**
 * This function generates the memory load specified by the coordinator task
 */
void* memory_generator_worker_task(void* args);

enum memory_generator_worker_state {
    MEMORY_STATE_NONE = 0,
    MEMORY_STATE_IDLE,
    MEMORY_STATE_READY,
    MEMORY_STATE_RUN,
    MEMORY_STATE_MAX,
};

#define ENUM_CASE(e, s)                                                        \
    case e:                                                                    \
        return s

static const char* _get_msg_type(enum memory_generator_msg_type type)
{
    switch (type) {
        ENUM_CASE(MEMORY_MSG_NONE, "None");
        ENUM_CASE(MEMORY_MSG_CONFIG, "Config");
        ENUM_CASE(MEMORY_MSG_LOAD, "Load");
        ENUM_CASE(MEMORY_MSG_START, "Start");
        ENUM_CASE(MEMORY_MSG_STOP, "Stop");
        ENUM_CASE(MEMORY_MSG_ABORT, "Abort");
    default:
        return ("Unknown");
    }
}

static const char* _get_worker_state(enum memory_generator_worker_state state)
{
    switch (state) {
        ENUM_CASE(MEMORY_STATE_NONE, "NONE");
        ENUM_CASE(MEMORY_STATE_IDLE, "IDLE");
        ENUM_CASE(MEMORY_STATE_READY, "READY");
        ENUM_CASE(MEMORY_STATE_RUN, "RUN");
    default:
        return ("UNKNOWN");
    }
}

#undef ENUM_CASE

struct memory_generator_worker_config
{
    struct op_packed_array* indexes;
    uint8_t* buffer;
    size_t op_index_min;
    size_t op_index_max;
    size_t op_block_size;
    size_t rate;
    enum icp_generator_pattern pattern;
};

struct icp_generator_scratch_area
{
    void* buffer;
    size_t size;
};

struct memory_generator_worker_data
{
    void* context;
    void* commands;
    struct icp_generator_scratch_area scratch;
    struct memory_generator_worker_config config;
    struct memory_generator_io_stats* stats;
    struct
    {
        size_t operations;
        size_t run_time;
        size_t sleep_time;
        double avg_rate;
    } total;
    size_t (*memory_spin)(const struct memory_generator_worker_config* config,
                          struct icp_generator_scratch_area* scratch,
                          uint64_t io_ops,
                          size_t* op_idx);
    enum memory_generator_worker_state state;
    struct
    {
        unsigned idx;
        unsigned count;
    } worker;
};

struct transition
{
    enum memory_generator_worker_state next_state;
    int (*handler)(struct memory_generator_worker_data* data,
                   struct memory_generator_control_msg* msg);
};

/**
 * Structure providing arguments for tasks
 */
struct icp_task_args
{
    void* context;                     /**< ZeroMQ context */
    char name[ICP_TASK_ARGS_NAME_MAX]; /**< Thread name for task */
    struct
    {
        const void* args; /**< (optional) argument for task */
        uint64_t flags;   /**< (optional) flags for task */
    } custom;
};

#endif
