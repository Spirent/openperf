% SPIPERF(1)
% https://github.com/Spirent/openperf
% January 2020

# NAME

spiperf - test network throughput, latency, and loss.

# SYNOPSIS

**spiperf -s** [_options_]

**spiperf -c** _server_ [_options_]

# DESCRIPTION

**spiperf** is a command line tool inspired by _iperf3_ that uses _OpenPerf_ as an underlying stack for increased performance, flexibility, and analytical capability. An emphasis has been placed on ease-of-use to allow minimal user input to run a meaningful test. At the same time a rich assortment of command-line options are provided so that discerning users may have enhanced control over their tests.

**spiperf** uses a client-server model for configuration and control purposes. A _server_ waits for a _client_ to connect and send configuration details. Once the _client_ starts the test, both sides collect statistics and present them to the user. This terminology is borrowed from _iperf_.

**spiperf** has two major modes of operation: stateful (TCP) and stateless (UDP) traffic.

In stateless mode **spiperf** configures traffic between _client_ and _server_ and provides statistics related to throughput, latency, and loss. **spiperf** measures one-way latency and loss using a special signature embedded within each packet.

Stateful mode has not been implemented at this time.

By default **spiperf** operates much like _iperf3_. Most _iperf3_ options are honored and results are presented in a comparable format. **spiperf** also provides an extended set of options, which unlock the full capabilities of **spiperf**.

To use **spiperf** user must first start one or two _OpenPerf_ instances.

# QUICK START

A basic **spiperf** test can be run with two simple commands:

```
spiperf --server
```

```
spiperf -c <server ip>
```

This runs a test that sends stateless (UDP) test data streams at 1Mbps for 10 seconds from the _client_ to the _server_ via their respective _OpenPerf_ instances.

Behind the scenes the _client_ instance will connect to the _server_ one via a dedicated **spiperf** control channel and share configuration information. Once both sides are configured the _client_ instance initiates test data stream transmission. While data stream(s) are running snapshot statistics are output periodically. Once traffic has finished sending the transmitting port will output end-of-test results that summarize the test.

# OPTIONS

## General Options

**-h**, **\--help**
:    Display usage summary.

**-V**, **\--verbose**
:    Increase verbosity of output. This option does not affect results output.

**-q**, **\--quiet**
:    Quiet output. This option does not affect results output. **spiperf** extension.

**-d**, **\--debug**
:    Output debug-level information. Mostly useful for developers, but can be helpful in debugging connectivity issues between **spiperf** and _OpenPerf_ instances.

**-o**, **\--openperf-url** _url_
:    Bind to _OpenPerf_ resource _url_. _url_ has the format <_host-address_>:<_host-port_>/<_resource-collection_>/<_resource-id_>. Here _host-address_ is the IP address or hostname of the _OpenPerf_ host, _host-port_ is the _OpenPerf_ API port, _resource-collection_ is either _ports_ or _interfaces_, and _resource-id_ is the resource identifier from _OpenPerf_. All fields are optional; **spiperf** will use sensible defaults for omitted values (i.e. _http://localhost:8080/_).

**-p**, **\--port** _n_
:    **spiperf** will listen on (server) or connect to (client) port _n_. This setting is for the **spiperf** to **spiperf** control connection.

**-v**, **\--version**
:    Output program version and exit.

**\--logfile** _file_
:    Output to logfile specified by _file_.

**\--force-flush**
:    Flush output after every line. May also be specified as **\--forceflush**.

**-i**, **\--interval** _n_
:    Output periodic results every _n_ seconds. Defaults to 1. Set to 0 to disable. Does not support time format suffixes.

**-f**, **\--format** _unit_
:    Report or process value in units _unit_. Supported values are: _K_, _M_, _G_, _T_ for _kilo-_, _mega-_, _giga-_, and _tera-_, respectively. For results if no option is specified **spiperf** will choose a reasonable default per value to minimize the number of digits in the output value.

**\--time-format** _unit_
:    Report or process value in _unit_ units. Supported values are: _n_, _u_, _m_, _s_ for _nano-_, _micro-_, _milli-_, and _seconds_ seconds, respectively. **spiperf** extension.

**-J**, **\--json**
:    Output results in _JSON_ format.

## Server Options

**-s**, **\--server**
:    Launch **spiperf** in server mode.

**-1**, **\--one-off**
:    Handle a single client test session and exit.

**-l**, **\--pidfile** _file_
:    Write process ID (PID) to _file_.

## General Client Options

**-c**, **\--client** _server_
:    Run **spiperf** in client mode connecting to **spiperf** server instance at _server_ (i.e. _spiperf-server.local_). _server_ may be one of IPv4 address, IPv6 address, or a hostname.

**-u**, **\--udp**
:    Send UDP (stateless) data streams. This is the default and only mode currently supported in **spiperf**. Included for compatibility with _iperf3_.

**\--connect-timeout** _n_
:    Wait _n_ milliseconds for server **spiperf** instance to respond to a connection request. Defaults to 3000. **spiperf** adds extended time unit suffixes _m_ and _s_.

**-b**, **\--bitrate** _n_[KMG]\[/_m_]
:    send data stream(s) at _n_ Kilo/Mega/Giga bits-per-second. Defaults to 1Mbps. If generating multiple data streams with the **-P** option, rate here is applied to each individually. Effective port rate is _n_ times value of **-P** option. Including a _/m_ will burst _m_ packets with minimal delay before waiting a sufficient time to allow rate to go back to specified value. A value of 0 will transmit as fast as possible. This option is mutually exclusive with the extended **spiperf** **\--upstream** and **\--downstream** options.

**-t**, **\--time** _n_
:    Run data stream(s) for _n_ seconds. Defaults to 10 seconds. Does not support extended time suffixes. Mutually exclusive with **-n** and **-k** options.

**-n**, **\--bytes** _n_[KMG]
:    Send _n_ bytes worth of data packets. Mutually exclusive with **-t** and **-k** options. **spiperf** interprets this the number of packet data bytes not just packet payload bytes (i.e. payload plus total header size). Minimum value is **-l** option value. If _n_ is not an even multiple of the **-l** option **spiperf** will output a warning and send at least _n_ bytes worth of data packets.

**-k**, **\--blockcount** _n_[KM]
:    Send _n_ data stream blocks (packets). Mutually exclusive with **-t** and **-n** options.

**\--client-port** _n_
:    Use _n_ as the TCP/UDP port for the client side of data stream(s). This will be either source or destination TCP/UDP port depending on data stream direction. Defaults to an ephemeral port in the _IANA_ range (49152 to 65535). May also be given as **\--cport**.

**\--server-port** _n_
:    Use _n_ as the TCP/UDP port for the server side of data stream(s). This will be either source or destination TCP/UDP port depending on data stream direction. Defaults to value of **-p** option (if present) or the default for **-p** option. **spiperf** extension.

**-P**, **\--parallel** _n_[_c_|_s_]
:    Transmit _n_ parallel traffic streams. Defaults to 1. Streams are differentiated from one another using different UDP/TCP ports. To generate source UDP/TCP ports **spiperf** will first pick a base port then increment by one until _n_ values have been generated. If the **\--client-port** option is specified **spiperf** will use that as the base port. By default **spiperf** steps the client side port. **spiperf** also supports two suffixes _c_ and _s_ to have **spiperf** step the client or server port, respectively. If the _s_ suffix and **\--server-port** are specified **spiperf** will use **\--server-port** value as the base port. Suffixes are a **spiperf** extension. If the base port plus step triggers a wrap-around condition an error will be generated.

**-R**, **\--reverse**
:    Reverse test direction (server sends to client). See also extended option **\--downstream**.

**-4**, **\--ipv4**
:    Use _IPv4_ data streams. See also **\--client-addrs** and **\--server-addrs** extended options for more flexible configuration. Added for compatibility with _iperf3_. May also be specified as **\--version4**.

**-6**, **\--ipv6**
:    Use _IPv6_ data streams. See also **\--client-addrs** and **\--server-addrs** extended options for more flexible configuration. Added for compatibility with _iperf3_. May also be specified as **\--version6**.

**\--client-addrs** _header_=_address_[,_header_=_address_]
:    Comma-separated list of protocol header=address pairs for the client side of data streams. List is processed left-to-right and applied to the packet in that order. Defaults to _Ethernet_,_IPv4_. See **\--help** output for a full list of supported headers. Case-insensitive. A non-exhaustive list includes: _Ethernet_, _VLAN_, _IPv4_, _IPv6_. Not all permutations are supported. Note: stateless traffic mode automatically appends a _UDP_ header to this list, with source and destination ports configured via **\--client-port** and **\--server-port** parameters. Supply the special value of _auto_ in place of an _address_ and **spiperf** will auto-generate a value. **spiperf** extension. Mutually exclusive with **-4** and **-6** options. Note: bind to an existing _OpenPerf_ _interface_ via the **-o** to preconfigure this information.

**\--server-addrs** _header_=_address_[,_header_=_address_]
:    Comma-separated list of protocol header=address pairs for the server side of data streams. List is processed left-to-right and applied to the packet in that order. Defaults to _Ethernet_,_IPv4_. See **\--help** output for a full list of supported headers. Case-insensitive. A non-exhaustive list includes: _Ethernet_, _VLAN_, _IPv4_, _IPv6_. Not all permutations are supported. Note: stateless traffic mode automatically appends a _UDP_ header to this list, with source and destination ports configured via **\--server-port** and **\--client-port** parameters.  Supply the special value of _auto_ in place of an _address_ and **spiperf** will auto-generate a value. **spiperf** extension. Mutually exclusive with **-4** and **-6** options. Note: bind to an existing _OpenPerf_ _interface_ via the **-o** to preconfigure this information.

**-S**, **\--tos** _n_
:    Use _n_ as IP Type of Service value in all data stream packets. Valid prefixes include _O_ (capital of the letter o) for octal and _0x_ for hex. No prefix denotes decimal format.

**\--dscp** _n_
:    Use _n_ as IP DSCP value in all data stream packets. For numerical values valid prefixes include _O_ (capital of the letter o) for octal and _0x_ for hex. No prefix denotes decimal format. Symbolic names are also accepted based on IANA DSCP registry names.

**-L**, **\--flowlabel** _n_
:    Use _n_ as IPv6 flow label in IPv6 data stream packets. Valid prefixes include _O_ (capital of the letter o) for octal and _0x_ for hex. No prefix denotes decimal format.

**\--get-server-output**
:    At the completion of a test retrieve output from the server and output.

## Stateless Options

**\--downstream** [_rate_]\[KMG]
:    Send traffic from _server_ to _client_. Optional rate parameter is in _bps_ and supports _K_, _M_, _G_ suffix. _rate_ defaults to 1Mbps. Related to **-b** for rate specification and **-R** for test traffic direction. **spiperf** extension.

**\--upstream** [_rate_]\[KMG]
:    Send traffic from _client_ to _server_. Optional rate parameter is in _bps_ and supports _K_, _M_, _G_ suffix. _rate_ defaults to 1Mbps. Related to **-b** for rate specification. **spiperf** extension. This direction is the default.

**-l**, **\--length** _n_
:    Send data stream packets of size _n_ bytes. Defaults to 128 bytes.

**\--imix** _length-code_[...]
:    Use _IMIX_ packet length distribution. Mutually exclusive with **-l**. Supports length codes _a_ to _g_ from RFC 6985 section 4. See _IMIX Length Table_ below for size-code mappings. Not supported with **-b** duration option. **spiperf** extension.

**\--fill-pattern** _bytes_
:    Fill data stream packet payloads with _bytes_ pattern. Must be in the format 0xYYYY where Y is a hex digit. **spiperf** extension.

**\--prbs**
:    Fill data stream packet payloads with bytes composed of a Pseudo-Random Bit Sequence (PRBS). **spiperf** extension.

**\--no-learning**
:    Skip ARP/ND for data streams. **spiperf** extension.

## Result Options

**\--extended-results**
:    Enable **spiperf** extended results set. Replaces default _iperf3_ style results. Alone this will display _terse_ result set. Use **\--runtime-results**, **\--final-results**, **\--analyze** options to further control result output. **spiperf** extension.

**\--runtime-results** _type_
:    Output _type_ runtime results. _type_ can be one of _none_, _terse_, _full_, _user_. Defaults to _terse_. _user_ type displays values specified by **\--analyze** parameter. Values _none_, _terse_, _full_ override **\--analyze** parameter. If omitted and **\--analyze** is present _type_ is _user_. _terse_ type analyzes _Transmit Bit Rate_, _Receive Bit Rate_, _Average Latency_, and _Drop (loss) Count_. **spiperf** extension. Implies **\--extended-results**.

**\--final-results** _type_
:    Output _type_ final results. _type_ can be one of _none_, _terse_, _full_, _user_. Defaults to _terse_. _user_ type displays values specified by **\--analyze** parameter. Values _none_, _terse_, _full_ override **\--analyze** parameter. If omitted and **\--analyze** is present _type_ is _user_. _terse_ type analyzes _Transmit Bit Rate_, _Receive Bit Rate_, _Average Latency_, and _Drop (loss) Count_. **spiperf** extension. Implies **\--extended-results**.

**\--analyze** _stat_ [,_stat_...]
:    List of statistics to analyze. A non-exhaustive list of values for _stat_ includes: _bit-rate_, _packet-rate_, _byte-count_, _packet-count_, _latency_, _jitter_, _sequence_. See _RESULTS_ section for a complete list. Statistics are output in the order specified. **spiperf** extension. Implies **\--extended-results**.

**\--raw-results-file** _file_
:    Output raw results for all analyzed statistics to _file_. If _file_ exists it is overwritten. **\--format** and **\--json** options do not affect output. Option is intended for _gnuplot_ visualization. See results section for additional details.


| Size (bytes) | IMIX Genome Code Letter |
|--------------|-------------------------|
| 64           | a                       |
| 128          | b                       |
| 256          | c                       |
| 512          | d                       |
| 1024         | e                       |
| 1280         | f                       |
| 1518         | g                       |

# EXIT STATUS

0
:    Successful test configuration and run

1
:    Unable to connect to _OpenPerf_ instance

2
:    Unable to connect to **spiperf** server instance (client only)

3
:    Connection to _OpenPerf_ instance was interrupted

4
:    **spiperf** server busy (client only)

5
:    Unable to bind to requested port (server only)

10
:    Unsupported rate requested

11
:    Invalid header list

12
:    Invalid client-side address(es)

13
:    Invalid server-side address(es)


20
:    ARP/ND did not succeed.

# EXAMPLES

This example shows an enhanced traffic configuration. This assumes the **spiperf** server is running on host _a.b.c.d_ port _4321_. It also assumes that the client's _OpenPerf_ instance is running on the same host using the default API port and that there's exactly one test port resource configured. Server configuration is omitted as it's not different from the example in the quick start section. Test will run 200Mbps of traffic downstream (server->client) and 20Mbps upstream for 2 minutes, and send a repeating packet size sequence of 64, 256, 128 bytes.

```
spiperf -c a.b.c.d:4321 -d 200M -u 20M -t 120 --imix acb
```

This example shows how to configure custom protocol headers. This assumes **spiperf** server instance is running on host _a.b.c.d_ port _4321_. It also assumes that the client's _OpenPerf_ instance is running on the same host using the default API port and that there's exactly one test port resource configured. Server configuration is omitted as it's not different from the example in the quick start section. Following table lists out protocols and addresses used. Arguments are on separate lines to better associate the address values with the intended header.

| Header   | Client Address    | Server Address    |
|----------|-------------------|-------------------|
| Ethernet | AA:BB:CC:DD:EE:FF | 00:11:22:33:44:55 |
| VLAN     | 17                | 200               |
| IPv4     | 192.168.1.24      | 10.0.0.1          |

```
spiperf -c a.b.c.d:4321 \
--client-addrs ethernet=aa:bb:cc:dd:ee:ff,vlan=17,ipv4=192.168.1.24 \
--server-addrs ethernet=00:11:22:33:44:vlan=55,ipv4=200,10.0.0.1
```

This example demonstrates how to specify _OpenPerf_ test port resources. The server is being configured to use the _OpenPerf_ instance on host _4.3.2.1_, port _6543_, and test port resource _test-port-one_. The server is being run on host _1.2.3.4_ and is listening on the default port. The client is being configured to use the _OpenPerf_ instance on the local host at the default port and using test port resource _port-two_.

```
spiperf -s -o 4.3.2.1:6543/ports/test-port-one

spiperf -c 1.2.3.4 -o /ports/port-two
```

This example demonstrates how to configure different result output. This assumes the **spiperf** server is running on host _a.b.c.d_ port _4321_. It also assumes that the client's _OpenPerf_ instance is running on the same host using the default API port and that there's exactly one test port resource configured. Server configuration is omitted as it's not different from the example in the quick start section. **spiperf** will output _Transmit Bit Rate_, _Receive Bit Rate_, _Transmit Packet Rate_, _Receive Packet Rate_, _Drop Packet Count_, _Out-of-Order Packet Count_ at runtime and when the test completes.

```
spiperf -c a.b.c.d:4321 --analyze bit-rate,packet-rate,sequence
```

# RESULTS

**spiperf** surfaces a rich subset of _OpenPerf_'s results and statistics. By default results output in the same format as _iperf3_. They also output in the same location, namely transmit results on the transmit side and receive results on the receive side. When using **spiperf**'s extended results, result values are collected from both transmit and receive and output together on the transmit side.

Transmit Bytes
:    Total number of bytes transmitted. Note: this measures packet data plus protocol headers.

Receive Bytes
:    Total number of bytes received. Note: this measures packet data plus protocol headers.

Transmit Bit Rate
:    Rate, in {Kilo,Mega,Giga} bits per second, at which test traffic is being transmitted.

Receive Bit Rate
:    Rate, in {Kilo,Mega,Giga} bits per second, at which test traffic is being received.

Transmit Packets
:    Number of test traffic packets that have been transmitted.

Receive Packets
:    Number of test traffic packets that have been received.

Transmit Packet Rate
:    Rate, in packets per second, at which test traffic packets are being transmitted.

Receive Packet Rate
:    Rate, in packets per second, at which test traffic packets are being received.

Latency
:    Elapsed time between transmission and receipt of a packet. Usually expressed in milliseconds (ms). Latency is the de-facto measurement of network delay (e.g. how long it takes packets to transit between _OpenPerf_ instances.) Higher values can indicate network issues.

Jitter
:    Variation between two successive Latency measurements. Usually expressed in milliseconds (ms). Higher values can indicate network issues.

Dropped Packet Count
:    Number of packets lost (e.g. transmitted but not received.) _OpenPerf_ leverages the signature field included with each packet to determine this value at runtime. A large value for this statistic can be indicative of a network that's over capacity or misconfigured. Other possible culprits include: poor connections leading to packet corruption, or even network device issues.

Out-of-Order Packet Count
:    Number of packets that were received out of order (e.g. received in a different order than transmitted.) Packet reordering causes interruptions and reduced performance of network protocols.

With such a long list of possible results, **spiperf** allows users to select which results they want to see.


| \--analyze   | Statistic Counter         |
|--------------|---------------------------|
| bit-rate     | Transmit Bit Rate         |
|              | Receive Bit Rate          |
| packet-rate  | Transmit Packet Rate      |
|              | Receive Packet Rate       |
| byte-count   | Transmit Byte Count       |
|              | Receive Byte Count        |
| packet-count | Transmit Packet Count     |
|              | Receive Packet Count      |
| latency      | Minimum Latency           |
|              | Average Latency           |
|              | Maximum Latency           |
| jitter       | Average Jitter            |
|              | Maximum Jitter            |
| sequence     | Dropped Packet Count      |
|              | Out-of-Order Packet Count |


For raw results **spiperf** outputs each statistic counter as a separate data set. Each row in a data set is the counter's value from a sample period. _latency_ and _jitter_ statistics have an extra data set. These extra data sets mirror the internal _t-digest_ data structure that tracked _latency_ and _jitter_ values for each packet. Each row in these data sets represents a data point from the _t-digest_ structure. Internally _t-digests_ store data points as mean-weight pairs.

# SEE ALSO

openperf(1)

