#ifndef _OP_UTILS_PREFETCH_FOR_EACH_HPP_
#define _OP_UTILS_PREFETCH_FOR_EACH_HPP_

#include <algorithm>
#include <iterator>

namespace openperf::utils {

/**
 * Algorigthm for iterating over an array and prefetch a few elements ahead.
 * @param cursor[in] Iterator for first item.
 * @param end[in] Iterator past last item.
 * @param prefetch_func[in]
 *   Function to call to prefetch the data required for func.
 * @param func[in]
 *   Function to call for each item.
 */
template <typename InputIterator, typename Function, typename PrefetchFunction>
void prefetch_for_each(InputIterator cursor,
                       InputIterator end,
                       PrefetchFunction&& prefetch_func,
                       Function&& func,
                       size_t ahead)
{
    InputIterator p_cursor = cursor;
    size_t n = std::distance(cursor, end);

    auto stop = cursor + std::min(n, ahead);
    while (p_cursor != stop) { prefetch_func(*p_cursor++); }

    while (p_cursor != end) {
        prefetch_func(*p_cursor++);
        func(*cursor++);
    }

    while (cursor != end) { func(*cursor++); }
}

template <typename InputIterator, typename Function, typename PrefetchFunction>
void prefetch_enumerate_for_each(InputIterator cursor,
                                 InputIterator end,
                                 PrefetchFunction&& prefetch_func,
                                 Function&& func,
                                 size_t ahead)
{
    InputIterator p_cursor = cursor;
    size_t n = std::distance(cursor, end);

    auto stop = cursor + std::min(n, ahead);
    while (p_cursor != stop) { prefetch_func(*p_cursor++); }

    auto idx = 0;
    while (p_cursor != end) {
        prefetch_func(*p_cursor++);
        func(idx++, *cursor++);
    }

    while (cursor != end) { func(idx++, *cursor++); }
}

} // namespace openperf::utils

#endif // _OP_UTILS_PREFETCH_FOR_EACH_HPP_
