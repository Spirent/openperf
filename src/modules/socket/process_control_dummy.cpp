#include <cassert>

#include "socket/process_control.hpp"

namespace openperf::socket::process_control {

bool enable_ptrace(FILE* output, std::optional<pid_t>)
{
    assert(output);

    fprintf(output, "Ptrace functionality is not supported on this platform\n");
    return (false);
}

} // namespace openperf::socket::process_control
