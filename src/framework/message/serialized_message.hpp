#ifndef _OP_MESSAGE_SERIALIZED_MESSAGE_HPP_
#define _OP_MESSAGE_SERIALIZED_MESSAGE_HPP_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "zmq.h"
#include "tl/expected.hpp"

namespace openperf::message {

struct serialized_message
{
    std::vector<zmq_msg_t> parts;

    serialized_message() = default;

    serialized_message(serialized_message&& other)
        : parts(std::move(other.parts))
    {}

    serialized_message& operator=(serialized_message&& other)
    {
        if (this != &other) { parts = std::move(other.parts); }
        return (*this);
    }

    ~serialized_message()
    {
        std::for_each(std::begin(parts), std::end(parts), [](auto& part) {
            zmq_msg_close(&part);
        });
    }
};

template <typename T>
std::enable_if_t<std::is_pointer_v<T>, T>
front_msg_data(serialized_message& msg)
{
    if (msg.parts.empty()) { return (nullptr); }

    auto& front = msg.parts.front();
    return (reinterpret_cast<T>(zmq_msg_data(&front)));
}

inline void pop_front(serialized_message& msg)
{
    auto& front = msg.parts.front();
    zmq_msg_close(&front);
    msg.parts.erase(std::begin(msg.parts));
}

template <typename T>
std::enable_if_t<std::is_copy_assignable<T>::value, int>
push(serialized_message& msg, const T& value)
{
    auto& part = msg.parts.emplace_back();

    if (auto error = zmq_msg_init_size(&part, sizeof(T)); error != 0) {
        msg.parts.pop_back();
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(&part));
    *ptr = value;
    return (0);
}

template <typename T>
auto push(serialized_message& msg, std::unique_ptr<T> value)
{
    auto& part = msg.parts.emplace_back();

    if (auto error = zmq_msg_init_size(&part, sizeof(T)); error != 0) {
        msg.parts.pop_back();
        return (error);
    }

    auto ptr = reinterpret_cast<T**>(zmq_msg_data(&part));
    *ptr = value.release();
    return (0);
}

template <typename T>
auto push(serialized_message& msg, const T* buffer, size_t length)
{
    auto& part = msg.parts.emplace_back();

    if (auto error = zmq_msg_init_size(&part, length); error != 0) {
        msg.parts.pop_back();
        return (error);
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(&part));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy_n(src, length, ptr);
    return (0);
}

// Need to use std::string here because other template functions
// can take precedence when std::string_view is used.
inline auto push(serialized_message& msg, const std::string& str)
{
    return push(msg, str.data(), str.size());
}

template <typename T>
std::enable_if_t<std::is_trivially_copyable<T>::value, int>
push(serialized_message& msg, const std::vector<T>& values)
{
    auto& part = msg.parts.emplace_back();
    auto length = sizeof(T) * values.size();

    if (auto error = zmq_msg_init_size(&part, length); error != 0) {
        msg.parts.pop_back();
        return (error);
    }

    // zmq data may not be aligned to type so safer to byte copy
    auto ptr = reinterpret_cast<char*>(zmq_msg_data(&part));
    auto src = reinterpret_cast<const char*>(values.data());
    std::copy_n(src, length, ptr);
    return (0);
}

template <typename T>
auto push(serialized_message& msg, std::vector<std::unique_ptr<T>>& values)
{
    auto& part = msg.parts.emplace_back();

    if (auto error = zmq_msg_init_size(&part, sizeof(T*) * values.size());
        error != 0) {
        msg.parts.pop_back();
        return (error);
    }

    auto cursor = reinterpret_cast<T**>(zmq_msg_data(&part));
    std::transform(std::begin(values), std::end(values), cursor, [](auto& ptr) {
        return (ptr.release());
    });
    return (0);
}

template <typename T>
std::enable_if_t<!std::is_pointer_v<T>, T> pop(serialized_message& msg)
{
    if (msg.parts.empty()) { return T{}; }

    auto& front = msg.parts.front();
    auto obj = *(reinterpret_cast<T*>(zmq_msg_data(&front)));
    pop_front(msg);
    return (obj);
}

template <typename T>
std::enable_if_t<std::is_pointer_v<T>, T> pop(serialized_message& msg)
{
    if (msg.parts.empty()) { return (nullptr); }

    auto& front = msg.parts.front();
    auto* ptr = *(reinterpret_cast<T*>(zmq_msg_data(&front)));
    pop_front(msg);
    return (ptr);
}

inline std::string pop_string(serialized_message& msg)
{
    if (msg.parts.empty()) { return {}; };

    auto& front = msg.parts.front();
    auto str = std::string(reinterpret_cast<char*>(zmq_msg_data(&front)),
                           zmq_msg_size(&front));
    pop_front(msg);
    return (str);
}

template <typename T>
std::enable_if_t<std::is_copy_assignable_v<T>, std::vector<T>>
pop_vector(serialized_message& msg)
{
    if (msg.parts.empty()) { return {}; }

    auto& front = msg.parts.front();
    auto length = zmq_msg_size(&front);
    auto size = length / sizeof(T);
    auto vec = std::vector<T>{};
    vec.resize(size);

    // zmq data may not be aligned to type so safer to byte copy
    auto src = reinterpret_cast<const char*>(zmq_msg_data(&front));
    auto ptr = reinterpret_cast<char*>(vec.data());
    std::copy_n(src, length, ptr);

    pop_front(msg);
    return (vec);
}

template <typename T>
std::vector<std::unique_ptr<T>> pop_unique_vector(serialized_message& msg)
{
    if (msg.parts.empty()) { return {}; }

    auto& front = msg.parts.front();
    auto size = zmq_msg_size(&front) / sizeof(T*);
    auto data = reinterpret_cast<T**>(zmq_msg_data(&front));

    auto vec = std::vector<std::unique_ptr<T>>{};
    std::for_each(
        data, data + size, [&](const auto& ptr) { vec.emplace_back(ptr); });
    pop_front(msg);
    return (vec);
}

inline int send(void* socket, serialized_message&& msg)
{
    if (any_of(std::begin(msg.parts),
               std::prev(std::end(msg.parts)),
               [&](auto&& part) {
                   return (zmq_msg_send(&part, socket, ZMQ_SNDMORE) == -1);
               })) {
        return (errno);
    }

    return (zmq_msg_send(&msg.parts.back(), socket, 0) == -1 ? errno : 0);
}

inline tl::expected<serialized_message, int> recv(void* socket, int flags)
{
    auto msg = serialized_message{};
    auto more = int{0};

    do {
        auto& part = msg.parts.emplace_back();
        if (zmq_msg_init(&part) == -1) { return (tl::make_unexpected(ENOMEM)); }
        if (zmq_msg_recv(&part, socket, flags) == -1) {
            return (tl::make_unexpected(errno));
        }
        auto more_size = sizeof(more);
        if (zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size) == -1) {
            return (tl::make_unexpected(errno));
        };
    } while (more);

    return (msg);
}

inline tl::expected<serialized_message, int> recv(void* socket)
{
    return (recv(socket, 0));
}

} // namespace openperf::message

#endif /* _OP_MESSAGE_SERIALIZED_MESSAGE_HPP_ */
