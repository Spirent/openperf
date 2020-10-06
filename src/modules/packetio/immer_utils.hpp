#ifndef _OP_PACKETIO_IMMER_UTILS_HPP_
#define _OP_PACKETIO_IMMER_UTILS_HPP_

#include <utility>

/*
 * Provide overloads to allow range based iteration of the pairs returned
 * by get_sources().  Since the lookup is done via the namespace of
 * the type, we need to add these functions to the 'immer' namespace.
 */
namespace immer {

template <typename Iterator>
Iterator begin(const std::pair<Iterator, Iterator>& range)
{
    return (range.first);
}

template <typename Iterator>
Iterator end(const std::pair<Iterator, Iterator>& range)
{
    return (range.second);
}

} // namespace immer

#endif /* _OP_PACKETIO_IMMER_UTILS_HPP_ */
