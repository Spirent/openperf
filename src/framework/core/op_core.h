#ifndef _OP_CORE_H_
#define _OP_CORE_H_

/**
 * @file
 */

/**
 * Convenience header to pull in all op_core headers
 */

#ifdef __cplusplus
#include "core/op_event_loop.hpp"
#include "core/op_init_factory.hpp"
#include "core/op_list.hpp"
#include "core/op_uuid.hpp"
#endif

#include "core/op_common.h"
#include "core/op_cpuset.h"
#include "core/op_event.h"
#include "core/op_event_loop.h"
#include "core/op_event_loop_utils.h"
#include "core/op_hashtab.h"
#include "core/op_init.h"
#include "core/op_list.h"
#include "core/op_log.h"
#include "core/op_modules.h"
#include "core/op_options.h"
#include "core/op_reference.h"
#include "core/op_socket.h"
#include "core/op_task.h"
#include "core/op_thread.h"

#endif /* _OP_CORE_H */
