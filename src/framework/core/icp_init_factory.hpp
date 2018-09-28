#ifndef _ICP_CORE_INIT_FACTORY_H_
#define _ICP_CORE_INIT_FACTORY_H_

#include <functional>
#include <memory>
#include <vector>

namespace icp {
namespace core {

/**
 * This class is based on the Unforgettable Factory Registration example
 * documented at http://www.nirfriedman.com/2018/04/29/unforgettable-factory.
 *
 * It uses the Curiously Repeating Template Pattern (CRTP) to allow
 * derived client objects to register their constructors with the factory
 * before main is called.
 */
template <class Base, class... Args>
class init_factory {
public:
    template <class ... T>
    static void make_all(std::vector<std::unique_ptr<Base>> &objects, T&&... args)
    {
        for (auto &ctor : ctors()) {
            objects.emplace_back(ctor(std::forward<T>(args)...));
        }
    }

    template <class T> struct registrar : Base {
        friend T;

        static bool registerT()
        {
            init_factory::ctors().emplace_back([](Args... args) -> std::unique_ptr<Base> {
                    return std::make_unique<T>(std::forward<Args>(args)...);
                });
            return (true);
        }

        static bool registered;

    private:
        registrar() : Base(Key{}) { (void)registered; }
    };

    friend Base;

private:
    class Key {
        Key() {};
        template <class T> friend struct registrar;
    };
    init_factory() = default;

    static auto &ctors() {
        static std::vector<std::function<std::unique_ptr<Base>(Args...)>> ctors;
        return ctors;
    }
};

template <class Base, class... Args>
template <class T>
bool init_factory<Base, Args...>::registrar<T>::registered =
    init_factory<Base, Args...>::registrar<T>::registerT();

}
}

#endif /* _ICP_CORE_INIT_FACTORY_H_ */
