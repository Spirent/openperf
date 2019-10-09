
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

    icp_thread_setname("icp_main");

    void *context = zmq_ctx_new();
    if (!context) {
        icp_exit("Could not initialize ZeroMQ context!");
    }

    /* Do initialization... */
    const char* argv[] = {"--config","config.yaml"};
    icp_init(context, 2, argv);

The `context` is a ZeroMQ message queue used to communicate with the Inception engine. The internal API client can be accessed using

	auto client = icp::packetio::internal::api::client(context);

Once the test is finihed, cleanup the stack using:

    /* ... then clean up and exit. */
    icp_halt(context);

## Packet Generator

Generating packets is a easy as 

	client.add_source("port0", source0);
	
Where `source0` is an Inception source:

	auto source0 = 	icp::packetio::packets::generic_source(test_source());

The test source needs to implement 
	
        virtual std::string id() const = 0;
        virtual bool active() const = 0;
        virtual uint16_t burst_size() const = 0;
        virtual uint16_t max_packet_length() const = 0;
        virtual packets_per_hour packet_rate() const = 0;
        virtual uint16_t transform(packet_buffer* input[], uint16_t input_length, packet_buffer* output[]) = 0;


Method | return type | Comment 
----- | ---- | ---
`Id` | string | Worker ID ( or core ID?)
`Active` | boolean | true if generation is active 
`burst_size` | uint16 | Needed for prealloc
`max_packet_length` | uint16 | Needed for prealloc
`packet_rate` | icp::units::rate | Packet rate
`transform` | ... | Actual packet generator

The `transform` is the core of the generator. A UDP/IP/ETH generator would look like this:

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

The `auto tmp = packets::to_data<test_packet>(packet);` will map the input packet into DPDK layers:

	struct test_packet {
	    struct rte_ether_hdr ether;
	    struct rte_ipv4_hdr  ipv4;
	    struct rte_udp_hdr   udp;
	} __attribute__((packed));

The `__attribute__((packed))` is needed to ensure that member fields have the smallest possible alignment.

Note that the application is responsible to computing the checksums of each layer (if there is checksum). For example:

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
 

Full generator source code available from [source-example/main.cpp](../targets source sink-example/main.cpp)

## Sink

Creating sinks, aka analysers, is as easy as generators:

	auto sink0 = icp::packetio::packets::generic_sink(test_sink());
    auto success = client.add_sink("port0", sink0);
    
The test_sink need to implement the following methods:

        virtual std::string id() const = 0;
        virtual uint16_t push(packet_buffer* const packets[], uint16_t length) = 0;

The `id` method is the same as for the generator, allowing to specify on which worker/core the sink is running. The `push` method is called for each received packet. For instance, this code will create statistics for received packet length.

	uint16_t push(icp::packetio::packets::packet_buffer* const packets[], uint16_t length) const
    {
        for(uint16_t i = 0; i < length; i++) {
            m_counter->add(icp::packetio::packets::length(packets[i]));
        }
        return (length);
    }

Full sink source code available from [sink-example/main.cpp](../targets/sink-example/main.cpp)

# Modules

## Socket Module

There are three folders _socket_, _client_  and _server_. The _client_ is what is linked with the application, while the server resides in the inception process.

### Client Initialization 

Upon startup (`icp::socket::api::client.init()`), the client will exchange an HELLO message with the server over local unix domain socket (`unix_socket` type [`SOCK_SEQPACKET`](http://urchin.earth.li/~twic/Sequenced_Packets_Over_Ordinary_TCP.html) to `/tmp/.com.spirent.inception/server`). The server responds with the server process ID and shared memory, as well as an optional pair of file descriptors to be used for subsequent communications.

When the client needs to create a new socket, it calls the socket equivalent wrapper [`client::socket(domain, type, protocol)`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/client/api_client.cpp#L435). The server allocates a _channel_ into the shared memory, a socket FD pair, and a socket ID.  

### Sending Data from the Client

Sending data over this socket (assuming UDP) will eventually call `client::sendmsg` with the destination `sockaddr` passed in the message header (`msg_name`). This method will send the data over the channel ([`channel.send`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/client/api_client.cpp#L593)), which in turn, calls the _dgram channel_ implementation [`dgram_channel::send`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/client/dgram_channel.cpp#L168). 


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
	
This last method transfers the data via the [`process_vm_writev`](https://linux.die.net/man/2/process_vm_writev) call (which copies memory between two processes without going through the kernel).
	
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
	
The sending process does not use the shared memory. Instead, it uses the [`sendq`](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/bipartite_ring.h) (implemented as a bipartite ring). The item type is `dgram_channel_item`, which contains a [pbuf_vec](https://github.com/SpirentOrion/inception-core/blob/master/src/modules/socket/pbuf_vec.cpp), which carries the payload and LWIP buf pointer as well as the payload length. From the client side, the puf is anonymised.

	struct dgram_channel_item {
	    std::optional<dgram_channel_addr> address;
	    pbuf_vec pvec;
	};

The `notify` is implemented in the `event_queue_producer` which the `dgram_channel` inherits from. At first glance, it looks quite complex:

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

But the most imporant is the `eventfd_write(fd(), 1)` which [_just_ sends](https://stackoverflow.com/questions/18557064/where-to-find-eventfd-write-documentation) an event of value `1` to the consumer (server). 

### Receiving Data from the Client

Receiving data (from a UDP client socket) is similar to sending data, at least in the first steps:
	
	
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
	
The actual recv implememtation also used memory copy between threads using `process_vm_readv`. The `ack_wait` is implemented in the _event queue_ and uses `eventfd_read` to wait for new events. The `ack_undo` pushes back and event on the _event queue_. 
	
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
	
The line `if (!recvq.available()) ack(); else ack_undo();` is to check to see if there are any remaining data to read.  If not, it clear any pending notification.  Otherwise, it makes sure a notification remains so the client reads the rest.

But, so, where is the shared memory?

