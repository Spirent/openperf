#ifndef _OP_CORE_INIT_FACTORY_HPP_
#define _OP_CORE_INIT_FACTORY_HPP_

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include <iostream>

namespace openperf {
namespace core {

/**
 * This class is based on the Unforgettable Factory Registration example
 * documented at http://www.nirfriedman.com/2018/04/29/unforgettable-factory.
 *
 * It uses the Curiously Repeating Template Pattern (CRTP) to allow
 * derived client objects to register their constructors with the factory
 * before main is called.
 */
template <class Base, class... Args> class init_factory
{
public:
    template <class... T>
    static void make_all(std::vector<std::unique_ptr<Base>>& objects,
                         T&&... args)
    {
        // Sort into priority order
        std::sort(begin(ctors()), end(ctors()),
                  [](const ranked_ctor& a, const ranked_ctor& b) -> bool {
                      return a.first < b.first;
                  });

        for (auto& ctor : ctors()) {
            objects.emplace_back(ctor.second(std::forward<T>(args)...));
        }
    }

    template <class T> struct registrar : Base
    {
        friend T;

        static bool registerT()
        {
            init_factory::ctors().emplace_back(std::make_pair(
                Priority<T>::value, [](Args... args) -> std::unique_ptr<Base> {
                    return std::make_unique<T>(std::forward<Args>(args)...);
                }));
            return (true);
        }

        static bool registered;

    private:
        registrar()
            : Base(Key{})
        {
            (void)registered;
        }
    };

    friend Base;

private:
    class Key
    {
        Key(){};
        template <class T> friend struct registrar;
    };

    /*
     * Use SFINAE to determine if the derived object has a static, public
     * priority value.  If so, we use it to determine the order of object
     * initialization.
     */
    template <class T> class Priority
    {
        template <class U,
                  class = typename std::enable_if_t<
                      !std::is_member_pointer<decltype(&U::priority)>::value>>
        static std::integral_constant<int, U::priority> check(int);

        template <class> static std::integral_constant<int, 99> check(...);

    public:
        static constexpr int value = decltype(check<T>(0))::value;
    };

    init_factory() = default;

    typedef std::pair<int, std::function<std::unique_ptr<Base>(Args...)>>
        ranked_ctor;

    static auto& ctors()
    {
        static std::vector<ranked_ctor> ctors;
        return ctors;
    }
};

template <class Base, class... Args>
template <class T>
bool init_factory<Base, Args...>::registrar<T>::registered =
    init_factory<Base, Args...>::registrar<T>::registerT();

} // namespace core
} // namespace openperf

#endif /* _OP_CORE_INIT_FACTORY_HPP_ */
