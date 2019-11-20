#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <charconv>
#include <fstream>
#include <memory>
#include <numeric>
#include <type_traits>

#include <sys/capability.h>
#include <sys/prctl.h>

#include "core/op_log.h"
#include "socket/process_control.h"

namespace openperf::socket::process_control {

static constexpr std::string_view yama_ptrace_file = "/proc/sys/kernel/yama/ptrace_scope";

static int read_ptrace_scope(FILE* output)
{
    assert(output);

    auto f = std::ifstream(yama_ptrace_file.data(), std::ifstream::in);
    if (!f.is_open()) {
        fprintf(output, "Could not determine ptrace scope; assuming classic ptrace permissions\n");
        return (0);
    }

    std::array<char, 1> input;
    f.read(input.data(), input.size());

    int value = 0;
    auto [p, ec] = std::from_chars(std::begin(input), std::end(input), value);

    return (ec == std::errc() ? value : 0);
}

static std::string to_string(cap_value_t cap_values[], size_t nb_cap_values)
{
    return (std::accumulate(cap_values, cap_values + nb_cap_values, std::string(),
                            [&](const std::string &s, auto& cap_value) -> std::string {
                                    return (s
                                            + (s.length() == 0 ? ""
                                               : nb_cap_values == 2 ? " and "
                                               : cap_values[nb_cap_values - 1] == cap_value ? ", and "
                                               : ", ")
                                            + cap_to_name(cap_value));
                            }));
}

static bool get_caps(FILE* output, cap_value_t cap_values[], size_t nb_cap_values)
{
    /* Use a nice, RAII wrapper for the caps pointer */
    auto caps = std::unique_ptr<std::remove_pointer_t<cap_t>, decltype(&cap_free)>(
        cap_get_proc(), cap_free);
    if (!caps) {
        fprintf(output, "Could not retrieve process capabilities: %s\n", strerror(errno));
        return (false);
    }

    /* Partition cap values into set/clear groups */
    auto cursor = std::partition(cap_values, cap_values + nb_cap_values,
                                 [&](auto& cap_value) {
                                     cap_flag_value_t flag = CAP_CLEAR;
                                     if (auto error = cap_get_flag(caps.get(), cap_value, CAP_EFFECTIVE, &flag);
                                         error != 0) {
                                         fprintf(output, "Could not read effective capability value for %s: %s\n",
                                                 cap_to_name(cap_value), strerror(errno));
                                     }
                                     return (flag == CAP_SET);
                                 });


    /* Set any needed capability flags */
    auto nb_unset_flags = std::distance(cursor, cap_values + nb_cap_values);
    if (nb_unset_flags) {
        if (auto error = cap_set_flag(caps.get(), CAP_EFFECTIVE, nb_unset_flags,
                                      cursor, CAP_SET); error != 0) {
            fprintf(output, "Could not set effective capability flag for %s: %s\n",
                    to_string(cursor, nb_unset_flags).c_str(), strerror(errno));
            return (false);
        }
    }

    return ((nb_unset_flags == 0 || cap_set_proc(caps.get()) == 0) ? true : false);
}

bool enable_ptrace(FILE* output, std::optional<pid_t> pid)
{
    assert(output);

    bool result = true;

    /* Processes must be "dumpable" in order for any ptrace attachment to succeed */
    if (prctl(PR_GET_DUMPABLE, 0, 0, 0, 0, 0) != 1) {
        if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) != 0) {
            fprintf(output, "Could not set dumpable flag: %s\n", strerror(errno));
            result = false;
        }
    }

    switch (read_ptrace_scope(output)) {
    case 0:
        /* Classic ptrace behavior; nothing to do */
        break;
    case 1:
        /* Process must have a predefined relationship with it's peer */
        if (prctl(PR_SET_PTRACER, pid ? *pid : PR_SET_PTRACER_ANY, 0, 0, 0) == -1) {
            fprintf(output, "Could not enable ptrace for pid %s: %s\n",
                    pid ? std::to_string(*pid).c_str() : "ANY",
                    strerror(errno));
            result = false;
        }
        break;
    case 2: {
        /* Process must have CAP_SYS_PTRACE capability */
        auto required_caps = std::array<cap_value_t, 1>{ CAP_SYS_PTRACE };
        if (!get_caps(output, required_caps.data(), required_caps.size())) {
            fprintf(output, "OpenPerf and its clients need the CAP_SYS_PTRACE capability. "
                    "Alternatively, consider changing the value of %.*s to 1.\n",
                    static_cast<int>(yama_ptrace_file.size()), yama_ptrace_file.data());
            result = false;
        }
        break;
    }
    default:
        /* ptrace is administratively disabled */
        fprintf(output, "Ptrace functionality is disabled. "
                "Please change %.*s to 0, 1, or 2.\n",
                static_cast<int>(yama_ptrace_file.size()), yama_ptrace_file.data());
        result = false;
    }

    return (result);
}

}
