/*
 * @file
 *
 * Generic memoization implementation.  The template magic required to memoize
 * generic functions is taken from Jim Porter's memo library at
 * https://github.com/jimporter/memo.
 *
 * However, the internal data structures have been changed from a std::map
 * to a set of fixed size arrays as the intent of this code is to speed
 * up associate array look-ups on critical code paths, e.g. we want to
 * turn an inexpensive hash table lookup into a really inexpensive
 * index into an array.  At least for the common case; caveat utilitor!
 */

/*
 * Copyright (c) 2014-2020, Jim Porter
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OP_UTILS_MEMOIZE_HPP_
#define _OP_UTILS_MEMOIZE_HPP_

#include <array>
#include <bitset>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>

#include "utils/hash_combine.hpp"

namespace openperf::utils {

namespace impl {

/* Equivalent to remove_cvref from C++20 */
template <typename T> struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T, typename U>
struct is_same_underlying_type
    : std::is_same<remove_cvref_t<T>, remove_cvref_t<U>>
{};

template <typename T, typename U>
inline constexpr bool is_same_underlying_type_v =
    is_same_underlying_type<T, U>::value;

/*
 * These templates translate various function signatures into something
 * we can recognize.
 */
template <typename T, typename Enable = void> struct function_signature
{};

template <typename T>
struct function_signature<T,
                          std::enable_if_t<std::is_class_v<remove_cvref_t<T>>>>
    : function_signature<decltype(&remove_cvref_t<T>::operator())>
{};

template <typename Object, typename Ret, typename... Args>
struct function_signature<Ret (Object::*)(Args...)>
{
    using type = Ret(Args...);
};

template <typename Object, typename Ret, typename... Args>
struct function_signature<Ret (Object::*)(Args...) const>
{
    using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct function_signature<Ret(Args...)>
{
    using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct function_signature<Ret (&)(Args...)>
{
    using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct function_signature<Ret (*)(Args...)>
{
    using type = Ret(Args...);
};

template <typename T>
using function_signature_t = typename function_signature<T>::type;

} // namespace impl

template <size_t Size, typename Signature, typename Function>
class flat_memoizer;

template <size_t Size, typename Ret, typename... Args, typename Function>
class flat_memoizer<Size, Ret(Args...), Function>
{
public:
    using function_type = Function;
    using function_signature = Ret(Args...);
    using tuple_type = std::tuple<std::remove_reference_t<Args>...>;
    using return_type = Ret;
    using return_reference = const return_type&;

    static_assert((Size & (Size - 1)) == 0); /* Size must be a power of two */
    static constexpr size_t mask = Size - 1;

    flat_memoizer(const function_type& f)
        : f_(f)
    {}

    template <typename... CallArgs>
    inline return_reference operator()(CallArgs&&... args)
    {
        return (call<std::conditional_t<
                    impl::is_same_underlying_type_v<CallArgs, Args>,
                    CallArgs&&,
                    impl::remove_cvref_t<Args>&&>...>(
            std::forward<CallArgs>(args)...));
    }

    template <typename... CallArgs>
    inline return_reference retry(CallArgs&&... args)
    {
        return (call_again<std::conditional_t<
                    impl::is_same_underlying_type_v<CallArgs, Args>,
                    CallArgs&&,
                    impl::remove_cvref_t<Args>&&>...>(
            std::forward<CallArgs>(args)...));
    }

private:
    template <typename... CallArgs>
    inline return_reference call(CallArgs... args)
    {
        auto key = std::forward_as_tuple(std::forward<CallArgs>(args)...);
        auto idx =
            std::hash<tuple_type>{}({std::forward<CallArgs>(args)...}) & mask;

        if (!flags_.test(idx) || keys_[idx] != key) {
            keys_[idx] = std::move(key);
            values_[idx] = f_(std::forward<decltype(args)>(args)...);
            flags_.set(idx);
        }

        return (values_[idx]);
    }

    template <typename... CallArgs>
    inline return_reference call_again(CallArgs... args)
    {
        auto key = std::forward_as_tuple(std::forward<CallArgs>(args)...);
        auto idx =
            std::hash<tuple_type>{}({std::forward<CallArgs>(args)...}) & mask;

        keys_[idx] = std::move(key);
        values_[idx] = f_(std::forward<decltype(args)>(args)...);
        flags_.set(idx);

        return (values_[idx]);
    }

    std::bitset<Size> flags_;              /* indicates if function has been
                                            * evaluated with the matching args */
    std::array<tuple_type, Size> keys_;    /* arguments used for function */
    std::array<return_type, Size> values_; /* value returned for function */
    function_type f_;                      /* function to call */
};

template <size_t Size, typename Signature, typename Function>
inline auto flat_memoize(Function&& f)
{
    return (
        flat_memoizer<Size, Signature, Function>(std::forward<Function>(f)));
}

template <size_t Size, typename Function> inline auto flat_memoize(Function&& f)
{
    return (flat_memoizer<Size, impl::function_signature_t<Function>, Function>(
        std::forward<Function>(f)));
}

} // namespace openperf::utils

#endif /* _OP_UTILS_MEMOIZE_HPP_ */
