#ifndef _ICP_PACKETIO_MESSAGE_UTILS_H_
#define _ICP_PACKETIO_MESSAGE_UTILS_H_

#include <algorithm>
#include <variant>
#include <zmq.h>

namespace icp::packetio::message {

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy(src, src + length, ptr);
    return (0);
}

template <typename T>
static T zmq_msg_data(const zmq_msg_t* msg)
{
    return (reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

template <typename T>
static size_t zmq_msg_size(const zmq_msg_t* msg)
{
    return (zmq_msg_size(const_cast<zmq_msg_t*>(msg))/sizeof(T));
}

/**
 * Templated function to retrieve the index for a specific type in a variant.
 * Helps make the switch statement when de-serializing a message a little
 * more intuitive.
 */
template <typename VariantType, typename T, std::size_t index = 0>
constexpr size_t variant_index() {
    if constexpr (std::is_same_v<std::variant_alternative_t<index, VariantType>, T>) {
        return (index);
    } else {
        return (variant_index<VariantType, T, index + 1>());
    }
}

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

}

#endif /* _ICP_PACKETIO_MESSAGE_UTILS_H_ */
