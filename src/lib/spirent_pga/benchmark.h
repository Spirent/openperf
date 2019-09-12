#ifndef _LIB_SPIRENT_PGA_BENCHMARK_H_
#define _LIB_SPIRENT_PGA_BENCHMARK_H_

#include <chrono>
#include <functional>
#include <vector>

namespace pga::benchmark {

/**
 * The benchmarking routine used here is an implementation of the method
 * described in "Robust Benchmarking in Noisy Environments" by Jiahao Chen
 * and Jarrett Revels.
 */

unsigned iterations();

using clock = std::chrono::high_resolution_clock;

template <typename Function, typename... Args>
std::chrono::nanoseconds measure_function_runtime(Function&& f, Args&&... args)
{
    std::vector<clock::duration> runtimes;
    runtimes.reserve(iterations());

    auto start = clock::now();
    unsigned count = 0;
    std::generate_n(std::back_inserter(runtimes),
                    iterations(),
                    [&](){
                        std::invoke(std::forward<Function>(f), std::forward<Args>(args)...);
                        return (clock::now() - start) / (++count);
                    });

    return (*std::min_element(begin(runtimes), end(runtimes)));
}

}

#endif /* _LIB_SPIRENT_PGA_BENCHMARK_H_ */
