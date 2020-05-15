#ifndef _OP_UTILS_SINGLETON_HPP_
#define _OP_UTILS_SINGLETON_HPP_

namespace openperf::utils {

template <typename T> class singleton
{
public:
    static T& instance()
    {
        static T instance;
        return (instance);
    }

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton) = delete;

protected:
    singleton() = default;
    ;
};

} // namespace openperf::utils

#endif /* _OP_UTILS_SINGLETON_HPP_ */
