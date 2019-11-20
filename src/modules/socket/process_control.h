#ifndef _OP_SOCKET_PROCESS_CONTROL_H_
#define _OP_SOCKET_PROCESS_CONTROL_H_

#include <cstdio>
#include <optional>

namespace openperf::socket::process_control {

/**
 * Enable appropriate ptrace permission based on the current host
 * environment.  We require an output FILE handle for error messages
 * because this function is not necessarily linked to the core framework
 * library, where the logging function is defined.
 *
 * @param[in] output
 *   FILE* for error messages
 * @param[in] pid
 *   - If set, specifies the pid that should be allowed to ptrace the caller.
 *   - If not set, all pids are allowed to ptrace the caller.
 *
 * @return
 *   - true:  ptrace is now enabled
 *   - false: ptrace could not be enabled
 */
bool enable_ptrace(FILE* output, std::optional<pid_t> pid = std::nullopt);

}

#endif /* _OP_SOCKET_PROCESS_CONTROL_H_ */
