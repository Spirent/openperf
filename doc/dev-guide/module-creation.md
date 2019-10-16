
# Inception Modules


## Declaration

Making a module a simple a straightforward. All which is needed is to declare the module using the `REGISTER_MODULE` macro:


```C++
REGISTER_MODULE(packetio,
    INIT_MODULE_INFO(
         "packetio",
         "Description of the module",
         icp::packetio::module_version
         ),
    new icp::packetio::service(),	// state
    nullptr, 						// pre_init
    icp_packetio_init, 				// init
    nullptr,						// post_init
    nullptr,						// start
    icp_packetio_fini				// finish
    );
}
```

The *state* is the module instance (i.e. the _this_ after creating the module instance).  The _init_ function most simple implementation consists is typecasting the _state_ to the module type, and calling the `init` method.


```C++
int icp_packetio_init(void *context, void *state)
{
    icp::packetio::service *s = reinterpret_cast<icp::packetio::service *>(state);
    s->init(context);
    s->start();
    return (0);
}
```

The addtional parameter, the _context_ is the *Inception* global context. It can be used for instance to communicate with other modules, to send messages via 0MQ, etc.

When terminating a module, the most simple implementation consist is just deleting the module instance:

```C++
void icp_packetio_fini(void *state)
{
    icp::packetio::service *s = reinterpret_cast<icp::packetio::service*>(state);
    delete s;
}
```

## Logging

Logging can be done using the `ICP_LOG` macro. For instance:

```C++
#include "core/icp_log.h"
ICP_LOG(ICP_LOG_DEBUG, "Reading from configuration file %s", file_name);
```

The different logging levels are:

   * `CRITICAL`: Fatal error condition
   * `ERROR`   : Non-fatal error condition
   * `WARNING` : Unexpected event or condition
   * `INFO`    : Informational messages
   * `DEBUG`   : Debugging messages
   * `TRACE`   : Trace level messages

All logs are available from the `inception.log` file. Each log line looks like:

```javascript
{
	"time": "2019-10-16T05:17:38Z",
	"level": "debug",
	"thread": "icp_packetio",
	"tag": "icp::packetio::dpdk::worker_controller::add_interface",
	"message": "Added interface with mac = 00:00:00:00:00:01 to port 0 (idx = 0)"
}
```

The tag is deducted from the `__PRETTY_FUNCTION__` c++ macro.

## ICP Context (aka 0MQ)

The most common usage for the _context_, in the Packet IO module seems to be for the creation of 0MQ sockets, eg:

```C++
const std::string endpoint = "inproc://icp_packetio_interface";
auto client = icp_socket_get_client(context, ZMQ_REQ, endpoint.c_str());
auto server = icp_socket_get_server(context, ZMQ_REP, endpoint.c_str());
```

The `icp_socket_get_server` function creates a [`zmq_socket`](http://api.zeromq.org/2-1:zmq-socket) and `zmq_bind` it to the _endpoint_. The `icp_socket_get_client`  also creates a `zmq_socket`, and then `zmq_connect` to the _endpoint_.

To send message, the client would use the following code:

```C++
json submit_request(void *client, json& request)
{
    std::vector<uint8_t> body = json::to_cbor(request);
    zmq_msg_t reply_msg;
    if (zmq_msg_init(&reply_msg) == -1
        || zmq_send(socket, body.data(), body.size(), 0) != body.size()
        || zmq_msg_recv(&reply_msg, socket, 0) == -1) {
        return {
            { "code", reply_code::ERROR },
            { "error", json_error(errno, zmq_strerror(errno)) }
        };
    }

    std::vector<uint8_t> reply(zmq_msg_data(&reply_msg),zmq_msg_data(&reply_msg) + zmq_msg_size(&reply_msg));
    zmq_msg_close(&reply_msg);
    return json::from_cbor(reply);
}
```

The process incoming message, this code can be used:

```C++
server::server(void* context, icp::core::event_loop& loop, void *arg)
{
	m_socket = icp_socket_get_server(context, ZMQ_REP, endpoint.c_str())
    struct icp_event_callbacks callbacks = {
        .on_read = _handle_rpc_request
    };
    loop.add(m_socket.get(), &callbacks, arg);
}

_handle_rpc_request(const icp_event_data *data, void *arg)
{
    zmq_msg_t request_msg;
    zmq_msg_init(&request_msg);


    while ((zmq_msg_recv(&request_msg, data->socket, ZMQ_DONTWAIT)) > 0) {

    	auto request_buffer_ptr = zmq_msg_data(&request_msg)
        std::vector<uint8_t> request_buffer(request_buffer_ptr, request_buffer_ptr + zmq_msg_size(&request_msg));

        json reply, request = json::from_cbor(request_buffer);

        //...
	  	//... process `request` and update `reply`
	  	//...

        std::vector<uint8_t> reply_buffer = json::to_cbor(reply);
        zmq_send(data->socket, reply_buffer.data(), reply_buffer.size(), 0)
    }

}
```

The `loop` is created by the module itself. It's type is the C++ class [`icp::core::event_loop`](https://github.com/SpirentOrion/inception-core/blob/master/src/framework/core/icp_event_loop.hpp#L11), which wraps the `icp_event_loop_xxx` functions. 

## Event Loops

Events loops are allocated (in plain C) using _icp_event_loop_allocate_. The underlying implementation is based on epoll or kqueue depending on the system.

Once created, event callbacks can be adeed using `icp_event_loop_add`, where callback are defined as:


```C++
struct icp_event_callbacks {
    union {
        struct {
            icp_event_callback *on_read;     /**< called on read event */
            icp_event_callback *on_write;    /**< called on write event */
            icp_event_callback *on_close;    /**< called on close event */
        };                                   /**< used by file and socket events */
        struct {
            icp_event_callback *on_timeout;  /**< called on timer expiration,
                                                  timer events only */
        };
    };
    icp_event_callback *on_delete;           /**< called when event is deleted */
};
```

Let's have a look at how a timer can be added. This can be done using the macro `icp_event_loop_add_timer`

```C++
int icp_event_loop_add_timer(struct icp_event_loop *loop,
      uint64_t timeout, /* Timeout in milliseconds */
      const struct icp_event_callbacks *callbacks, /*Callback*/
      void *arg, /* Callback argument */
      uint32_t *timeout_id /* Timeout ID. needed to delete or update the timer */
      ); 
```

Timer can ne stopped using `icp_event_loop_disable_timer` and it's callback can be updated using `icp_event_loop_update_timer_cb`


## 
