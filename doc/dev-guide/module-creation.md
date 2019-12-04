

# OpenPerf Modules


## Declaration

Making a module a simple straightforward. The first step is to declare it using the `REGISTER_MODULE` macro:


```C++
REGISTER_MODULE(packetio,
    INIT_MODULE_INFO(
         "packetio",
         "Description of the module",
         openperf::packetio::module_version
         ),
    new openperf::packetio::service(), // state
    nullptr,            // pre_init
    op_packetio_init,        // init
    nullptr,            // post_init
    nullptr,            // start
    op_packetio_fini       // finish
    );
}
```

The *state* is the module instance (i.e. the _this_ after creating the module instance).  The _init_ function most simple implementation consists of typecasting the _state_ to the module type and calling the `init` method.


```C++
int op_packetio_init(void *context, void *state)
{
    openperf::packetio::service *s = reinterpret_cast<openperf::packetio::service *>(state);
    s->init(context);
    s->start();
    return (0);
}
```

The addtional parameter, the _context_ is *the* 0MQ context.

When terminating a module, the most simple implementation consists is just deleting the module instance:

```C++
void op_packetio_fini(void *state)
{
    openperf::packetio::service *s = reinterpret_cast<openperf::packetio::service*>(state);
    delete s;
}
```

## Options

Modules can declare options which can be specified in the config.yaml file. For instance, for the API module:

```C++
MAKE_OPTION_DATA(api, NULL,
     MAKE_OPT("API service port", "modules.api.port", 'p', OP_OPTION_TYPE_LONG),
     );

REGISTER_CLI_OPTIONS(api)
```

It then possible to declare the port in the yaml config this way:

```yaml
modules:
  api:
    port: 9000
```

The module implementation can access the declared property this way:

```C++
#include "config/op_config_file.h"
auto port = openperf::config::file::op_config_get_param<OP_OPTION_TYPE_LONG>("modules.api.port");
```


## Logging

Logging can be done using the `OP_LOG` macro. For instance:

```C++
#include "core/op_log.h"
OP_LOG(OP_LOG_DEBUG, "Reading from configuration file %s", file_name);
```

The different logging levels are:

   * `CRITICAL`: Fatal error condition
   * `ERROR`   : Non-fatal error condition
   * `WARNING` : Unexpected event or condition
   * `INFO`    : Informational messages
   * `DEBUG`   : Debugging messages
   * `TRACE`   : Trace level messages

All logs are available from the `openperf.log` file. Each log line looks like:

```javascript
{
  "time": "2019-10-16T05:17:38Z",
  "level": "debug",
  "thread": "op_packetio",
  "tag": "openperf::packetio::dpdk::worker_controller::add_interface",
  "message": "Added interface with mac = 00:00:00:00:00:01 to port 0 (idx = 0)"
}
```

The tag is deducted from the `__PRETTY_FUNCTION__` c++ macro.

## Message Queue

Using the 0MQ _context_, it is possible to create 0MQ sockets:

```C++
const std::string endpoint = "inproc://op_packetio_interface";
auto client = op_socket_get_client(context, ZMQ_REQ, endpoint.c_str());
auto server = op_socket_get_server(context, ZMQ_REP, endpoint.c_str());
```

The `op_socket_get_server` function creates a [`zmq_socket`](http://api.zeromq.org/2-1:zmq-socket) and `zmq_bind` it to the _endpoint_. The `op_socket_get_client`  also creates a `zmq_socket`, and then `zmq_connect` to the _endpoint_.

To send message, the client would use the following code:

```C++
tl::expected<reply_msg, int> do_request(void* socket, const request_msg& request)
{
    if (send_message(socket, serialize_request(request)) != 0) {
        return (tl::make_unexpected(errno));
    }
    return (recv_message(socket).and_then(deserialize_reply));
}
```

To process an incoming message, the module uses an event loop, that executes a callback each time a new message is received:

```C++
server::server(void* context, openperf::core::event_loop& loop, void *arg)
{
  m_socket = op_socket_get_server(context, ZMQ_REP, endpoint.c_str())
    struct op_event_callbacks callbacks = {
        .on_read = handle_rpc_request
    };
    loop.add(m_socket.get(), &callbacks, arg);
}
```

The `loop` is instanciated by the module. Its type is the C++ class [`openperf::core::event_loop`](https://github.com/Spirent/openperf/blob/d8d89dfc8f2545572f917d0903706a3577a0a76b/src/framework/core/op_event_loop.hpp#L11), which wraps the `op_event_loop_xxx` functions.

The callback typically deserializes the message and then uses an `std::visit` to switch to the correct message handler.

```C++
handle_rpc_request(const op_event_data* data, void* arg)
{
    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT).and_then(deserialize_request)) {

        auto handle_visitor = [&](auto& request_msg) -> reply_msg {
            return (handle_request(server->workers(), request_msg));
        };
        auto reply = std::visit(handle_visitor, *request);

        send_message(data->socket, serialize_reply(reply));
    }
}
```

The `recv_message`  returns a  [tl::expected](https://github.com/TartanLlama/expected) promise.

Note that for this code to work, the module needs to implement the [transmogrification](https://github.com/Spirent/openperf/blob/d8d89dfc8f2545572f917d0903706a3577a0a76b/src/modules/packetio/internal_transmogrify.cpp#L16) for the serialized objects.

```C++
struct serialized_msg {
    zmq_msg_t type;
    zmq_msg_t data;
};
```

Yes, one serialized message is implemented as two multi-part 0MQ messages. When sent, they are concatenated, using the option `ZMQ_SNDMORE`, which _specifies that the message being sent is a multi-part message and that further message parts are to follow_.

The module must implement the serialization:

```C++
serialized_msg serialize_request(const request_msg& msg)
{
    serialized_msg serialized;
    message::zmq_msg_init(&serialized.type, msg.index();
    std::visit(message::overloaded_visitor(
          [&](const request_sink_add& sink_add) {
              return (message::zmq_msg_init(&serialized.data, std::addressof(sink_add.data), sizeof(sink_add.data)));
          },
          [&](const request_sink_del& sink_del) {
               return (message::zmq_msg_init(&serialized.data, std::addressof(sink_del.data), sizeof(sink_del.data)));
          }),
       msg));
    return (serialized);
}
```

And the deseralization:

```C++
tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(message::zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case message::variant_index<request_msg, request_sink_add>():
        return (request_sink_add{ *(message::zmq_msg_data<sink_data*>(&msg.data)) });
    ...
    }
    return (tl::make_unexpected(EINVAL));
}
```

The `using index_type = decltype(std::declval<request_msg>().index());`  is a smart _modern C++_ [way](https://arne-mertz.de/2017/01/decltype-declval/) to associate a `std::variant<...>` to an index. The `message::variant_index` is also using modern C++ - detailled explanations [here](https://stackoverflow.com/a/52303671).

```C++
template <typename VariantType, typename T, std::size_t index = 0>
constexpr size_t variant_index() {
    if constexpr (std::is_same_v<std::variant_alternative_t<index, VariantType>, T>) {
        return (index);
    } else {
        return (variant_index<VariantType, T, index + 1>());
    }
}
```

## Event Loops

Events loops are allocated (in plain C) using _op_event_loop_allocate_. The underlying implementation is based on epoll or kqueue depending on the system.

Once created, event callbacks can be adeed using `op_event_loop_add`, where callback are defined as:


```C++
struct op_event_callbacks {
    union {
        struct {
            op_event_callback *on_read;     /**< called on read event */
            op_event_callback *on_write;    /**< called on write event */
            op_event_callback *on_close;    /**< called on close event */
        };                                   /**< used by file and socket events */
        struct {
            op_event_callback *on_timeout;  /**< called on timer expiration,
                                                  timer events only */
        };
    };
    op_event_callback *on_delete;           /**< called when event is deleted */
};
```

Let's have a look at how a timer can be added. This can be done using the macro `op_event_loop_add_timer`

```C++
int op_event_loop_add_timer(struct op_event_loop *loop,
      uint64_t timeout, /* Timeout in milliseconds */
      const struct op_event_callbacks *callbacks, /*Callback*/
      void *arg, /* Callback argument */
      uint32_t *timeout_id /* Timeout ID. needed to delete or update the timer */
      );
```

Timer can be stopped using `op_event_loop_disable_timer` and it's callback can be updated using `op_event_loop_update_timer_cb`


For some reason to be understood, the event loop is not widely used in the module - only Packet IO is using it, and only for 0MQ message read callback. The packet IO worker implementation is even having its own `generic_event_loop` implementation
