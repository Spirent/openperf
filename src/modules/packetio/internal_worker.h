#ifndef _ICP_PACKETIO_INTERNAL_WORKER_H_
#define _ICP_PACKETIO_INTERNAL_WORKER_H_

/**
 * @file
 *
 * This file contains functions intended to be run from within packetio
 * worker threads.  Generally, these should be called from generic_{sink,source}
 * code that runs within the worker thread context.
 *
 * Calling these functions *outside* of a worker thread context is an error.
 */

namespace icp::packetio::internal::worker {

/**
 * Retrieve the worker thread id
 *
 * @return
 *   value between [0, id_max)
 */
unsigned get_id();

/**
 * Retrieve the worker thread's numa node
 *
 * @return
 *   value between [0, numa_node_max)
 */
unsigned get_numa_node();

}

#endif /* _ICP_PACKETIO_INTERNAL_WORKER_H_ */
