#ifndef _PREFETCH_FOR_EACH_HPP_
#define _PREFETCH_FOR_EACH_HPP_

/**
 * Algorigthm for iterating over an array and prefetch a few elements ahead.
 * @param first[in] Iterator for first item.
 * @param last[in] Iterator past last item.
 * @param prefetch_func[in]
 *   Function to call to prefetch the data required for func.
 * @param func[in]
 *   Function to call for each item.
 */
template <typename InputIterator, typename Function, typename PrefetchFunction>
void prefetch_for_each(InputIterator first,
                       InputIterator last,
                       PrefetchFunction prefetch_func,
                       Function func,
                       size_t ahead)
{
    InputIterator prefetch_it = first, it = first;
    size_t n = std::distance(first, last);

    // Prefetch initial data
    for (auto end = first + std::min(n, ahead); prefetch_it != end;
         ++prefetch_it) {
        prefetch_func(*prefetch_it);
    }

    // Process items and continue to prefetch
    for (; prefetch_it != last; ++it, ++prefetch_it) {
        prefetch_func(*prefetch_it);
        func(*it);
    }

    // All done prefetching so just need to process the rest
    for (; it != last; ++it) { func(*it); }
}

#endif // _PREFETCH_FOR_EACH_HPP_