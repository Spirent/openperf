#ifndef _ICP_API_ROUTE_HANDLER_H_
#define _ICP_API_ROUTE_HANDLER_H_

#include <functional>
#include <memory>
#include <vector>

#include "pistache/router.h"

namespace icp {
namespace api {
namespace route {

/**
 * This class is based on the Unforgettable Factory Registration example
 * documented at http://www.nirfriedman.com/2018/04/29/unforgettable-factory.
 *
 * It uses the Curiously Repeating Template Pattern (CRTP) to allow
 * client code to register route handling objects with the REST API endpoint
 * during program initialization.
 */
template <class Base, class... Args>
class factory {
public:
    template <class ... T>
    static void make_all_handlers(std::vector<std::unique_ptr<Base>> &handlers, T&&... args)
    {
        for (auto &ctor : ctors()) {
            handlers.emplace_back(ctor(std::forward<T>(args)...));
        }
    }

    template <class T> struct registrar : Base {
        friend T;

        static bool registerT()
        {
            factory::ctors().emplace_back([](Args... args) -> std::unique_ptr<Base> {
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
    factory() = default;

    static auto &ctors() {
        static std::vector<std::function<std::unique_ptr<Base>(Args...)>> ctors;
        return ctors;
    }
};

template <class Base, class... Args>
template <class T>
bool factory<Base, Args...>::registrar<T>::registered =
    factory<Base, Args...>::registrar<T>::registerT();

struct handler : factory<handler, void *, Pistache::Rest::Router &>
{
    handler(Key) {};
};

}
}
}

#endif /* _ICP_API_ROUTE_HANDLER_ */
