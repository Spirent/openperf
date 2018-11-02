#ifndef _ICP_CORE_H_
#define _ICP_CORE_H_

/**
 * @file
 */

/**
 * Convenience header to pull in all icp_core headers
 */

#ifdef __cplusplus
#include "core/icp_event_loop.hpp"
#include "core/icp_init_factory.hpp"
#endif

#include "core/icp_common.h"
#include "core/icp_cpuset.h"
#include "core/icp_event.h"
#include "core/icp_event_loop.h"
#include "core/icp_event_loop_utils.h"
#include "core/icp_init.h"
#include "core/icp_list.h"
#include "core/icp_log.h"
#include "core/icp_modules.h"
#include "core/icp_options.h"
#include "core/icp_reference.h"
#include "core/icp_socket.h"
#include "core/icp_string.h"
#include "core/icp_task.h"
#include "core/icp_thread.h"

#endif /* _ICP_CORE_H */
