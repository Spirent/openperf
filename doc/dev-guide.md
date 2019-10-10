
# Building Inception


First, checkout the git folder

	git clone https://github.com/SpirentOrion/inception-core

Then download the sub-modules dependencies

	make -C inception-core deps

Then, build the docker image to get all tools installed (needed for C++17 compilation):

	docker build -t inception-builder inception-core

Then build the inception code from within the docker container:

	docker run -it --privileged -v ./inception-core:/project inception-builder:latest /bin/bash -c "make -C /project all"

If all is fine, you should see something like

	sudo -n /sbin/setcap CAP_IPC_LOCK,CAP_NET_RAW,CAP_SYS_ADMIN,CAP_DAC_OVERRIDE,CAP_SYS_PTRACE=epi /project/build/inception-linux-x86_64-testing/bin/inception
	make[1]: Leaving directory '/project/targets/inception'
	make[1]: Entering directory '/project/targets/libicp-shim'
	clang++ -flto=thin -shared -o /project/build/libicp-shim-linux-x86_64-testing/lib/libicp-shim.so -fuse-ld=lld -static-libstdc++ -static-libgcc -shared -L/project/build/libicp-shim-linux-x86_64-testing/lib /project/build/libicp-shim-linux-x86_64-testing/obj/icp-shim.o /project/build/libicp-shim-linux-x86_64-testing/obj/libc_wrapper.o -licp_socket_client -lrt -ldl
	make[1]: Leaving directory '/project/targets/libicp-shim'
	root@8f97d154f160:/project#
	root@8f97d154f160:/project#

Finally, run the tests

	docker run -it --privileged -v ./inception-core:/project inception-builder:latest /bin/bash -c "make -C /project test"

For verbose test, you can run the following command within the docker container: 

	cd /project/tests/aat;
	env/bin/mamba spec --format=documentation

You will be able to see the following logs:

	Modules,
	  list,
	    all,
	      ✓ it returns list of modules (1.3598 seconds)
	      unsupported method,
	        ✓ it returns 405
	  get,
	    known module,
	      packetio
	        ✓ it succeeds
	    non-existent module,
	      ✓ it returns 404
	    unsupported method,
	      ✓ it returns 405
	    invalid module name,
	      ✓ it returns 400	


# Internal API 

This section gives some practical example for creating an using the Inception stack usign the Internal APIS (i.e. without LD-Preload Shim)

![Internal Components](images/internal-api.png)

## Creating the ICP Stack

```C++	
void main(int argc, const char ** argv) {
	icp_thread_setname("icp_main");
	
	void *context = zmq_ctx_new();
	if (!context) {
	    icp_exit("Could not initialize ZeroMQ context!");
	}
	
	/* Do initialization... */
	icp_init(context, argc, argv);
}
```

The application should be started with the arguments `--config config.yaml` in order for `icp_init` to locate the configuration file.

The `context` is a ZeroMQ message queue used to communicate with the Inception engine. The internal API client can be accessed using

```C++	
auto client = icp::packetio::internal::api::client(context);
```

Once the test is finished, cleanup the stack using:

```C++	
/* ... then clean up and exit. */
icp_halt(context);
```

Note that in the following examples, this configuration is uses (is exposes `port0` and it's associated _interface_).

```
modules:
  packetio:
    dpdk:
      port-ids:
        port0: port0

resources:
  /interfaces/interface0:
    port_id: port0
    config:
      protocols:
        - eth:
            mac_address: "00:10:94:ae:d6:ab"
        - ipv4:
            method: static
            static:
              address: "198.18.25.1"
              prefix_length: 15
              gateway: "198.18.0.1"
```

## Packet Generator

Generating packets is a easy as 

```C++	
client.add_source("port0", source0);
```
	
Where `port0` must be defined in the configuration file (`config.yaml`) and where `source0` is an Inception source:

```C++	
auto source0 = 	icp::packetio::packets::generic_source(test_source());
```

The test source needs to implement the following methods:
	
```C++	
virtual std::string id() const = 0;
virtual bool active() const = 0;
virtual uint16_t burst_size() const = 0;
virtual uint16_t max_packet_length() const = 0;
virtual packets_per_hour packet_rate() const = 0;
virtual uint16_t transform(packet_buffer* input[], uint16_t input_length, packet_buffer* output[]) = 0;
```

As well as the move semantics for those two methods (which is defined as the default semantics since the test source only has copyable data members).

```C++
test_source(test_source&& other) = default;
test_source& operator=(test_source&& other) = default;
```

Method | return type | Comment 
----- | ---- | ---
`Id` | string | Worker ID ( or core ID?)
`Active` | boolean | true if generation is active 
`burst_size` | uint16 | Needed for prealloc
`max_packet_length` | uint16 | Needed for prealloc
`packet_rate` | icp::units::rate | Packet rate
`transform` | ... | Actual packet generator

The `transform` is the core of the generator. A UDP/IP/ETH generator would look like this:

```C++	
using namespace icp::packetio;
using namespace icp::packetio::packets;
	
uint16_t transform(packet_buffer* input[], uint16_t count, packet_buffer* output[])
{
    auto src_mac = mac_address{ 0x00, 0xff, 0xaa, 0xff, 0xaa, 0xff };
    auto dst_mac = mac_address{ 0x00, 0xaa, 0xff, 0xaa, 0xff, 0xaa };
    auto src_ip = ipv4_address("198.18.25.1");
    auto dst_ip = ipv4_address("198.19.25.1");

	std::for_each(input, input + count,[&](auto packet) {
      auto tmp = packets::to_data<test_packet>(packet);

      initialize_eth_header(tmp->ether, src_mac, dst_mac, RTE_ETHER_TYPE_IPV4);
      initialize_ipv4_header(tmp->ipv4, src_ip, dst_ip, 26);
      initialize_udp_header(tmp->udp, 3357, 3357, 18);

      packets::length(packet, 60);
      *output++ = packet;
    });
    return count;
}
```

The `auto tmp = packets::to_data<test_packet>(packet);` will map the input packet into DPDK layers:

```C++	
struct test_packet {
    struct rte_ether_hdr ether;
    struct rte_ipv4_hdr  ipv4;
    struct rte_udp_hdr   udp;
} __attribute__((packed));
```

The `__attribute__((packed))` is needed to ensure that member fields have the smallest possible alignment.

Note that the application is responsible to computing the checksums of each layer (if there is checksum). For example:

```C++	
static void initialize_ipv4_header(rte_ipv4_hdr& ip_hdr,
                                   ipv4_address& src_addr, ipv4_address& dst_addr,
                                   uint16_t pkt_data_len)
{
    auto pkt_len = pkt_data_len + sizeof(ip_hdr);
    /*
     * Initialize IP header.
     */
    ip_hdr.version_ihl     = 0x45;
    ip_hdr.type_of_service = 0;
    ip_hdr.fragment_offset = 0;
    ip_hdr.time_to_live    = 0x10;
    ip_hdr.next_proto_id   = IPPROTO_UDP;
    ip_hdr.hdr_checksum    = 0;
    ip_hdr.packet_id       = 0;
    ip_hdr.total_length    = htons(pkt_len);
    ip_hdr.src_addr        = htonl(src_addr.data());
    ip_hdr.dst_addr        = htonl(dst_addr.data());
	
    /*
     * Compute IP header checksum.
     */
    auto ptr16 = reinterpret_cast<uint16_t*>(std::addressof(ip_hdr));
    unsigned ip_cksum = std::accumulate(ptr16, ptr16 + 10, 0U,
                                        [](unsigned sum, auto chunk){ return (sum + chunk); });
	
    /*
     * Reduce 32 bit checksum to 16 bits and complement it.
     */
    ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) + (ip_cksum & 0x0000FFFF);
    ip_cksum = (~ip_cksum) & 0x0000FFFF;
    ip_hdr.hdr_checksum = static_cast<uint16_t>(ip_cksum == 0 ? 0xFFFF : ip_cksum);
}
```

Full generator source code available from [source-example/main.cpp](../targets source sink-example/main.cpp)

## Sink

Creating sinks, aka analysers, is as easy as generators:

```C++	
auto sink0 = icp::packetio::packets::generic_sink(test_sink());
auto success = client.add_sink("port0", sink0);
```
    
The test_sink need to implement the following methods:

```C++	
virtual std::string id() const = 0;
virtual uint16_t push(packet_buffer* const packets[], uint16_t length) = 0;
```

As well as the move semantics for those two methods (which is more complex that the test_source, since the sink has to cope with non-copyable members, such as the pointer to the counter (`std::unique_ptr<packet_counter<uint64_t>> m_counter`)).


```C++
test_sink(test_sink&& other)
test_sink& operator=(test_sink&& other)
```


The `id` method is the same as for the generator, allowing to specify on which worker/core the sink is running. The `push` method is called for each received packet. For instance, this code will create statistics for received packet length.

```C++	
uint16_t push(icp::packetio::packets::packet_buffer* const packets[], uint16_t length) const
{
    for(uint16_t i = 0; i < length; i++) {
        m_counter->add(icp::packetio::packets::length(packets[i]));
    }
    return (length);
}
```

Full sink source code available from [sink-example/main.cpp](../targets/sink-example/main.cpp)

# Modules

## Socket Module - UDP

There are three folders _socket_, _client_  and _server_. The _client_ is what is linked with the application, while the server resides in the inception process.

### Client Initialization 

Upon startup (`icp::socket::api::client.init()`), the client will exchange an HELLO message with the server over local unix domain socket (`unix_socket` type [`SOCK_SEQPACKET`](http://urchin.earth.li/~twic/Sequenced_Packets_Over_Ordinary_TCP.html) to `/tmp/.com.spirent.inception/server`). The server responds with the server process ID and shared memory, as well as an optional pair of file descriptors to be used for subsequent communications.

When the client needs to create a new socket, it calls the socket equivalent wrapper [`client::socket(domain, type, protocol)`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/client/api_client.cpp#L435). The server allocates a _channel_ into the shared memory, a socket FD pair, and a socket ID.  

### Sending Data from the Client

Sending data over this socket (assuming UDP) will eventually call `client::sendmsg` with the destination `sockaddr` passed in the message header (`msg_name`). This method will send the data over the channel ([`channel.send`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/client/api_client.cpp#L593)), which in turn, calls the _dgram channel_ implementation [`dgram_channel::send`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/client/dgram_channel.cpp#L168). 


```C++	
ssize_t client::sendto(int s, const void *dataptr, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    auto iov = iovec{
        .iov_base = const_cast<void*>(dataptr),
        .iov_len = len
    };
	
    auto msg = msghdr{
        .msg_name = const_cast<sockaddr*>(to),
        .msg_namelen = tolen,
        .msg_iov = &iov,
        .msg_iovlen = 1
    };
	
    return (sendmsg(s, &msg, flags));
}
	
	
ssize_t client::sendmsg(int s, const struct msghdr *message, int flags)
{
    auto& [id, channel] = m_channels.find(s)->second;
    
    channel.send(m_server_pid, message->msg_iov, message->msg_iovlen, 
    	flags, reinterpret_cast<const sockaddr*>(message->msg_name));
}
```
	
This last method transfers the data via the [`process_vm_writev`](https://linux.die.net/man/2/process_vm_writev) system-call (the data moves directly between the address spaces of the two processes, without passing through kernel space).
	
```C++	
dgram_channel::send(pid_t pid, iov[], iovcnt,..., sockaddr *to)
{
    auto item = sendq.unpack();
    item->address = to_addr(to);
	
    iovec writevec = iovec{ 
    	.iov_base = const_cast<void*>(item->pvec.payload()),
        .iov_len = item->pvec.len() 
    };
    process_vm_writev(pid, iov, iovcnt, &writevec, 1, 0);
	
    item->pvec.len(result);
    sendq.repack();
	
    notify();
	...	
}
```
	
The sending process does not use the shared memory. Instead, it uses the [`sendq`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/bipartite_ring.h) (implemented as a bipartite ring). The item type is `dgram_channel_item`, which contains a [pbuf_vec](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/pbuf_vec.cpp), which carries the payload and LWIP buf pointer as well as the payload length. From the client side, the puf is anonymised.

```C++	
struct dgram_channel_item {
    std::optional<dgram_channel_addr> address;
    pbuf_vec pvec;
};
```

The `notify` is implemented in the `event_queue_producer` which the `dgram_channel` inherits from. At first glance, it looks quite complex:

```C++	
template <typename Derived>
int event_queue_producer<Derived>::notify()
{
    if (!count()) {
        write_idx().fetch_add(1, std::memory_order_release);
        if (auto err = eventfd_write(fd(), 1); err != 0) {
            write_idx().fetch_sub(1, std::memory_order_release);
            return (errno);
        }
    }
    return (0);
}
```

But the most imporant is the `eventfd_write(fd(), 1)` which [_just_ sends](https://stackoverflow.com/questions/18557064/where-to-find-eventfd-write-documentation) an event of value `1` to the consumer (server). 

The other thing to note is that the atomic counters are used the same way as head/tail indexes of a ring buffer, but here we call them read_idx and write_idx. If read == write, then there are no outstanding notifications for the event fd. If read != write, then there are notifications. We do this to minimize our syscalls for reading/writing the fd's. So, that eventfd_write function is only called if we know for a fact that the other side is idle (and hence needs a wake up).


### Receiving Data from the Client

Receiving data (from a UDP client socket) is similar to sending data, at least in the first steps:
	
```C++	
ssize_t client::recvfrom(int s, void *mem, size_t len, int flags,
                 struct sockaddr *from, socklen_t *fromlen)
{
    auto iov = iovec{
        .iov_base = mem,
        .iov_len = len
    };
	
    auto msg = msghdr{
        .msg_name = from,
        .msg_namelen = (fromlen ? *fromlen : 0),
        .msg_iov = &iov,
        .msg_iovlen = 1
    };
	
    auto to_return = recvmsg(s, &msg, flags);
    if (fromlen && to_return != -1) *fromlen = msg.msg_namelen;
    return (to_return);
	
}
	
ssize_t client::recvmsg(int s, struct msghdr *message, int flags)
{
    auto& [id, channel] = m_channels.find(s)->second;
    channel.recv(
    	m_server_pid, message->msg_iov, message->msg_iovlen, 
    		flags, message->msg_name, &message->msg_namelen);
    ..
}
```
	
The actual recv implememtation also used memory copy between threads using `process_vm_readv`. The `ack_wait` is implemented in the _event queue_ and uses `eventfd_read` to wait for new events. The `ack_undo` pushes back an event on the _event queue_. 
	
```C++
dgram_channel::recv(pid_t pid, iovec iov[], size_t iovcnt, int, ...)
{
    while (!recvq.available()) {
        if (auto error = ack_wait(); error != 0) {
            return (tl::make_unexpected(error));
        }
    }
	
    auto item = (flags & MSG_PEEK ? recvq.peek() : recvq.unpack());
	
    auto readvec = iovec { 
    	.iov_base = const_cast<void*>(item->pvec.payload()),
       .iov_len = item->pvec.len() 
    };
    process_vm_readv(pid, iov, iovcnt, &readvec, 1, 0);
    recvq.repack();
	
    notify();
	
    if (!recvq.available()) ack(); else ack_undo();
	
    ...	
}
```
	
The line `if (!recvq.available()) ack(); else ack_undo();` is to check to see if there are any remaining data to read.  If not, it clear any pending notification.  Otherwise, it makes sure a notification remains so the client reads the rest.

But, so, where is the shared memory? To understand it, let's first dig into the server code.

### Server Initialization 

When creating a socket (UDP in the following example), the server will execute `api_handler::handle_request_socket`, which will, in turn create the actual socket using [`make_socket`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/server/socket_utils.cpp#L15)

	make_socket(m_allocator, request.domain, request.type, request.protocol);
	
When the allocator is a `icp::memory::allocator` working with the shared memory. The make socket function will eventually call the _right_ socket implementation, in our case, the [`udp_socket`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/server/udp_socket.cpp#L56)

```C++
udp_socket::udp_socket(icp::socket::server::allocator& allocator, int flags)
{
    m_channel = new (allocator.allocate(sizeof(dgram_channel)))
    dgram_channel_deleter = &allocator
    dgram_channel = flags
    m_pcb = udp_new()      
               
    if (!m_pcb) {
        throw std::runtime_error("Out of UDP pcb's!");
    }
	
    udp_recv(m_pcb.get(), &udp_receive, m_channel.get());
}
```

The `udp_new ` returns an actual LWIP pcb. The `dragm_channel` is defined as:

```C++
typedef bipartite_ring<dgram_channel_item, api::socket_queue_length> dgram_ring;

class dgram_channel : public event_queue_consumer<dgram_channel>
                    , public event_queue_producer<dgram_channel>
{
	dgram_ring sendq;  /* from client to stack */  
	api::socket_fd_pair client_fds;                
	std::atomic_uint64_t tx_fd_write_idx;          
	std::atomic_uint64_t rx_fd_read_idx;           
	dgram_ring recvq;  /* from stack to client */  
	api::socket_fd_pair server_fds;                
	std::atomic_uint64_t tx_fd_read_idx;           
	std::atomic_uint64_t rx_fd_write_idx;          
	std::atomic_int socket_flags;
```

The magic is that both send and receive queues are statically allocated as part of the `dgram_channel` -  but yet, those two queues are only holding pointers to the payload. So where is the payload actually allocated? To find the answer, let's check the receiving process.

### Receiving Data from the Server

The LWIP recv handler `udp_receive` is registered when the socket is created.

```C++
static void udp_receive(void* arg, udp_pcb* pcb, pbuf* p, const ip_addr_t* addr, in_port_t port)
{
    auto channel = reinterpret_cast<dgram_channel*>(arg);
    if (!channel->send(p, reinterpret_cast<const dgram_ip_addr*>(addr), htons(port))) {
        pbuf_free(p);
    }
}
```

Note that the `dgram_channel` referenced here is the server dgram channel. It is a seperate implementation of the client dgram channel, howerver both implementation share the same interpretation of the same underlying data. The udp receive handler then calls the chanel `send` method, which pushes a `dgram_channel_item`


```C++
bool dgram_channel::send(const pbuf *pbuf, const dgram_ip_addr* addr, in_port_t port)
{
    free_spent_pbufs(recvq);
    bool pushed = recvq.enqueue( dgram_channel_item{ 
    	.address = dgram_channel_addr(addr, port),
    	.pvec = pbuf_vec(const_cast<pbuf*>(p), p->payload, p->len) 
    });
    if (pushed) notify();
    return (pushed);
}
``` 

The LWIP states that _the callback is responsible for freeing the pbuf if it's not used any more_, which is what `free_spent_pbufs` is _asynchronously_ doing.  To be considered as a `spent pbuf`, the client will first have to call `recv`. 

So, in conclusion, for UDP, the shared memory is only used for the channel and queues, but not for the payload, as described on the component diagram below:

![Internal Components](images/socket-module-udp.png)

## Socket Module - TCP

As explained in the section before, the UDP socket implementation does not use the shared memory for passing the protocol buffers (pbuf's). This is however not the case for TCP, while use shared memory for both queue and payloads.

### Sending data from the Client

The TCP client stack is implemented as the `stream_channel`. When sending data, the client will eventually run the following code:

```C++
tl::expected<size_t, int> stream_channel::send(pid_t pid, iov[], iovcount, ...) {

	auto flags = socket_flags.load(std::memory_order_relaxed);
	auto isNonBlocking = flag & EFD_NONBLOCK

	size_t buf_available = 0;
    while ((buf_available = writable()) == 0) {
         isNonBlocking ? block() : block_wait()
    }

    auto written = std::accumulate(
    	iov, iov + iovcnt, 0UL, [&](size_t x, const iovec& iov) {
          auto v = write_and_notify(iov.iov_base, iov.iov_len, 
          	[&]() { notify(); })
    	  return (x + v);
    });

    if (written == buf_available && isNonBlocking && !writable()) {
        block();   /* pre-emptive block */
    };

    notify();

    return (written);
}
```

The first part `while (writable()==0)` waits for space to be available in the ring buffer.  The second part, `std::accumulate` over `write_and_notify`  writes the actualy data from the io vector, and, for each io chunk, notifies the server over the ring buffer. Last, the `if (written == buf_available ...) block()` is used when the buffer is full after writing, to give a change for the client to get scheduled (_to be confirmed_). 

An interresting scenario is to understand what happens if there is not enough space available in the buffer, when calling the `write_and_notify`? Well, just like the standard socket implmentation, `write_and_notify` will only write as many bytes as it can, and return the actual written size. (_Note: could there be a atomicity bug, because the std::accumlate does not prevents from buffer to be read while iterating the write?_)

The main difference with UDP is that UDP uses `sendq` (type `dgram_ring`) and  `process_vm_writev` while TCP uses `write_and_notify` only; The _simplified_ TCP socket structure is the following


```C++
class stream_channel : public circular_buffer_consumer<stream_channel>
                     , public circular_buffer_producer<stream_channel>
                     , public event_queue_consumer<stream_channel>
                     , public event_queue_producer<stream_channel>
{
	buffer tx_buffer;
	buffer rx_buffer;
	api::socket_fd_pair client_fds;              
	std::atomic_int socket_error;                
	std::atomic_int socket_flags;                
	api::socket_fd_pair server_fds;
	void* allocator;
```


When the TCP socket is created, the server allocates the buffer from the shared memory:

```
stream_channel::stream_channel(int flags, icp::socket::server::allocator& allocator)
    : tx_buffer(allocator.allocate(init_buffer_size), init_buffer_size)
    , rx_buffer(allocator.allocate(init_buffer_size), init_buffer_size)
    , ...
```

Where `init_buffer_size` is defined as *128 kb*. The `buffer` class is surprisingly simple, but yet very efficiently designed:

```C++
struct buffer {
    uint64_t len;
    memory::offset_ptr<uint8_t> ptr;
    buffer(uint8_t* ptr_, uint64_t len_) : ptr(ptr_) , len(len_) {}
    buffer() : ptr(memory::offset_ptr<uint8_t>::uninitialized()) {}
};
```

Note that all of the buffer data is kept in two cache aligned structs so that the read and write threads will not destructively share the data. The first struct is written to exclusively by the server thread.  The second struct is written to exclusively by the client thread.
 
### Receiving data from the Server

After a stream socket has accepted a new incomming connection, it receives the data via the `do_lwip_recv` method:

```C++
int tcp_socket::do_lwip_recv(pbuf *p, int err)
{
    /* Store the incoming pbuf in our buffer */
    m_recvq.push(p);

    if (p->flags & PBUF_FLAG_PUSH
        || m_recvq.length() > m_channel->send_available()) {
        do_tcp_receive_all(m_pcb.get(), *m_channel, m_recvq);
    }
}
```

The `m_recvq` is a pbuf queue. The second part `if (p->flags & PBUF_FLAG_PUSH...` initiate a copy to the client's buffer if the TCP sender sent a PUSH flag or there are more than enough data to fill the client's receive buffer.

The `do_tcp_receive_all` eventually calls the `channel.send` and then frees the pbuf (LWIP function `tcp_recved`). The channel `send` method is efficiently relying on the `circular_buffer_producer::write` method to copy the data:

```C++
size_t stream_channel::send(const iovec iov[], size_t iovcnt)
{
    auto written = write(iov, iovcnt);
    notify();
    return (written);
}
```

The `write` method makes efficient copy using the `dpdk::memcpy` helper. From the client side, when notified of available data, the client will call the `read_and_notify` which also copies the data using `dpdk::memcpy`.

### Sending data from the Server

When the client has notified of new available outgoing data (which it previously copied to the output buffer), the server needs to send this data to the LWIP stack. It does it via the `do_tcp_transmit` method (_very_ simpified in the code below):

```C++
	static size_t do_tcp_transmit(tcp_pcb* pcb, const void* ptr, size_t length, ...)
{
	size_t written = 0;
	auto to_write = std::min(length, pcb->snd_buf);

	const auto flags = TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE;
	
	if (tcp_write(pcb, ptr, to_write, flags) != ERR_OK) {
	    return (written);
	}
		
	return to_write;
}

```

The server implementation uses `TCP_WRITE_FLAG_COPY`, meaning that the client data on the TX circular buffer can be released immediately.

The following component diagram summurizes the _stream channel_ data flow.



![Internal Components](images/socket-module-tcp.png)
