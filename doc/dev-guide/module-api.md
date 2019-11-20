
# API Module

## API handlers registration

The API module is based on [pistache]([http://pistache.io/quickstart](http://pistache.io/quickstart)) for serving HTTP requests. The standard Pistache HTTP request looks like this:

```cpp
class HelloHandler : public Http::Handler {
public:
    HTTP_PROTOTYPE(HelloHandler)
    void onRequest(const Http::Request& request, Http::ResponseWriter response) {
         response.send(Http::Code::Ok, "Hello, World");
    }
};
```

The API module uses a more advanced struct based on routers:

```cpp
class handler : public handler_factory::registrar<handler> {
public:
    handler(void *context, Rest::Router& router) {
      Rest::Routes::Get(router, "/modules", Rest::Routes::bind(&handler::list_modules, this));
    }
    void list_modules(const Rest::Request& request, Http::ResponseWriter response) {
      json jmodules = json::array();
      ...
      response.send(Http::Code::Ok, jmodules.dump());
    }
};
```
The API module alos uses the nice [_init factory_](http://www.nirfriedman.com/2018/04/29/unforgettable-factory) design pattern (implemented as the `handler_init`). The factory is declared in this way:

```cpp
struct handler_init : openperf::core::init_factory<handler, void *, Pistache::Rest::Router &>
{
    handler(Key) {};
    virtual ~handler() = default;
};
```

There's no need to register the handlers _manually_, the init factory takes care of this. Once all the handlers are declared, they can be instantiated in this way, where `context, router` is the argument passed to each constructor.

```cpp
handlers std::vector<std::unique_ptr<handler_init>>
handler_init::make_all(handlers, context, router);
```

Note that message handlers are not only declared in the API module. Each module can have its handlers, such as the [Packet IO interface module](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/packetio/interface_handler.cpp).

## Configuration Mapping

Once the routes and messages and configured, the next step for the API server is to read the `config.yaml` and instantiate the interface, ports, etc... accordingly. This is done in the `op_config_file_process_resources` function.

For each of the `resources` declared in the configuration, the function first validated the content, convert the resource definition to _JSON_ and then calls the HTTP server _internally_ using `openperf::api::client::internal_api_post`

The _internal API post_ implementation is also based on Pistache:

```cpp
static auto internal_api_request(Http::RequestBuilder &request_builder, const std::string &body)
{
    auto response = request_builder.body(body).send();

    std::pair<Http::Code, std::string> result;
    response.then(
      [&result](Http::Response response) {
          result.first  = response.code();
          result.second = response.body();
      },
      Async::IgnoreException);

    Async::Barrier<Http::Response> barrier(response);
    barrier.wait_for(std::chrono::seconds(request_timeout));

    return result;
}
```
Pistache is using its implementation of async promises. The `Async::Barrier`.`wait_for` is part of the Pistache library, and using a [std::condition_variable](https://github.com/oktal/pistache/blob/master/include/pistache/async.h#L1109) to wait for the promise to resolve (or fail).
