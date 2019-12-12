% SPIPERF(1)
% Spirent Communications
% December 2019

# NAME

spiperf - test network throughput, latency, and loss.

# SYNOPSIS

**spiperf -s** [_options_]

**spiperf -c** _server_ [_options_]

# DESCRIPTION

**spiperf** is a command line tool inspired by _iperf3_ that uses _OpenPerf_ as an underlying stack for increased performance, flexibility, and analytical capability. An emphasis has been placed on ease-of-use to allow minimal user input to run a meaningful test. At the same time a rich assortment of command-line options are provided so that discerning users may have enhanced control over their tests.

**spiperf** uses a client-server model for configuration and control purposes. A `server` waits for a `client` to connect and send configuration details. Once the test is started by the `client`, both sides collect statistics and present them to the user. This terminology is borrowed from _iperf_.

**spiperf** has two major modes of operation: stateful (TCP) and stateless (UDP) traffic.

In stateless mode **spiperf** configures traffic between `client` and `server` and provides statistics related to throughput, latency, and loss, if any. Using a special signature embedded within each packet it is able to determine latency and loss in one direction without the need for round-trip transmission.

Stateful mode has not been implemented at this time.

To use **spiperf** user must first start two _OpenPerf_ instances, one each for the `client` and `server`.

# QUICK START

The simplest **spiperf** test can be run with two simple commands:

```
spiperf --server
```

```
spiperf -c <server ip>
```

This will result in stateless (UDP) test traffic being sent at 1Mbps for 10 seconds from the `client` to the `server` via their respective _OpenPerf_ instances.

Behind the scenes the `client` instance will connect to the `server` one and share configuration information. Once both sides are configured the `client` instance will initiate test traffic transmission. While traffic is running the transmitting **spiperf** instance will periodically output snapshot statistics. Unlike _iperf3_ these statistics are a snapshot of the entire test up until the sample is taken, not of a fixed interval. Once traffic has finished sending the transmitting port will output end-of-test results that summarize the test.

# OPTIONS

## General Options

**-h**, **\--help**
:    Display usage summary.

**-V**, **\--verbose**
:    Increase verbosity of output. Adding additional **-V** further increases verbosity. This option does not affect results output.

**-q**, **\--quiet**
:    Quiet output. This option does not affect results output.

**-o**, **\--openperf-url** _url_
:    Bind to _OpenPerf_ resource _url_. _url_ has the format <_host-ip_>:<_host-port_>/<_port-id_>. Here _host-ip_ is the IP address or hostname of the _OpenPerf_ host, _host-port_ is the _OpenPerf_ API port, and _port-id_ is the test port resource identifier from _OpenPerf_. All fields are optional; **spiperf** will use sensible defaults for omitted values. In This case _host-ip_ will default to _localhost_, _host-port_ will be the default _OpenPerf_ API port, and _port-id_ will default to the sole port resource.

**-p**, **\--port** _n_
:    **spiperf** will listen on (server) or connect to (client) port _n_.

**-v**, **\--version**
:    Output program version.

**\--logfile** _file_
:    Output to logfile specified by _file_.

**\--forceflush**
:    Flush output after every line.

**-4**, **\--version4**
:    Force **spiperf** to use only IPv4 for control (client-server) connection. 

**-6**, **\--version6**
:    Force **spiperf** to use only IPv6 for control (client-server) connection.

## Server Options

**-s**, **\--server**
:    Launch **spiperf** in server mode.

**-1**, **\--one-off**
:    Handle a single client test session and exit.

**-l**, **\--pidfile** _file_
:    Write process ID (PID) to _file_.

## General Client Options

**-c**, **\--client** _server_
:    Run **spiperf** in client mode connecting to **spiperf** server instance at address _server_.

**-t**, **\--time** _n_
:    Run traffic for _n_ seconds. Defaults to 10 seconds.

**\--connect-timeout** _n_
:    Wait _n_ milliseconds for server **spiperf** instance to respond to a connection request. Defaults to 3000 ms.

**\--cport** _n_
:    Use _n_ as TCP/UDP destination port for test traffic.

**\--headers** _header_[,_header_]
:    Comma-separated list of protocol headers to use in test traffic packets. List is processed left-to-right and applied to the packet in that order. Defaults to _Ethernet_,_IPv4_. See **\--help** output for a full list of supported headers. Case-insensitive. A non-exhaustive list includes: _Ethernet_, _VLAN_, _IPv4_, _IPv6_. Not all permutations are supported. Note: stateless traffic mode automatically appends a _UDP_ header to this list.

**\--client-addrs** _address_[,_address_]
:    Comma-separated list of client-side addresses to use in test traffic packets. List is processed left-to-right and applied to the packet in that order. A special value of _auto_ can be supplied in place of a specific address and **spiperf** will auto-generate that address. Note: does not cover automatically-inserted header; use _\--cport_ instead.

**\--server-addrs** _address_[,_address_]
:    Comma-separated list of server-side addresses to use in test traffic packets. List is processed left-to-right and applied to the packet in that order. A special value of _auto_ can be supplied in place of a specific address and **spiperf** will auto-generate that address. Note: does not cover automatically-inserted header; use _\--cport_ instead.

**-S**, **\--tos** _n_
:    Use _n_ as IP Type of Service value in test traffic packets.

**\--dscp** _n_
:    Use _n_ as IP DSCP value in test traffic packets.

**-L**, **\--flowlabel** _n_
:    Use _n_ as IPv6 flow label in IPv6 test traffic packets.

**\--get-server-output**
:    At the completion of a test retrieve output from the server and output.

## Stateless Options

**-d**, **\--downstream** [_rate_]
:    Send traffic from _server_ to _client_. Optional rate parameter is in _bps_ and supports _K_, _M_, _G_ suffix. _rate_ defaults to 1Mbps.

**-u**, **\--upstream** [_rate_]
:    Send traffic from _client_ to _server_. Optional rate parameter is in _bps_ and supports _K_, _M_, _G_ suffix. _rate_ defaults to 1Mbps.

**-k**, **\--blockcount** _n_
:    Send _n_ test traffic blocks (packets).

**-l**, **\--packet-length** _n_
:    Send test traffic packets of size _n_ bytes. Defaults to 128 bytes.

**\--imix** _length-code_[...]
:    Use _IMIX_ frame length distribution. Mutually exclusive with **-l**. Supports length codes _a_ to _g_ from RFC 6985 section 4. See _IMIX Length Table_ below for size-code mappings.
 
**\--fill-pattern** _bytes_
:    Fill test traffic packets payload with _bytes_ pattern. Must be in the format 0xYYYY where Y is a hex digit.

**\--prbs **
:    Fill packet payload portion with bytes composed of a Pseudo-Random Bit Sequence (PRBS).

**\--no-learning**
:    Skip ARP/ND for test traffic.

**-P**, **\--parallel** _n_
:    Transmit _n_ parallel traffic streams. Streams are differentiated from one another using different source UDP/TCP ports. Specified traffic rate is divided equally among streams. Defaults to 1.

## Result Options

**-i**, **\--interval** _n_
:    Output periodic results every _n_ seconds. Defaults to 1.

**-J**, **\--json**
:    Output results in _json_ format.

**\--no-runtime-results**
:    Do not output periodic results during test execution.

**\--no-final-results**
:    Do not output end-of-test summary results.

**\--terse-runtime-results**
:    Periodically output a useful but not exhaustive set of results during test execution. This list includes _Transmit Bit Rate_, _Receive Bit Rate_, _Average Latency_, _Drop Packet Count_. This is the default.

**\--terse-final-results**
:    Output a useful but not exhaustive set of results upon test completion. This list includes _Transmit Bit Rate_, _Receive Bit Rate_, _Average Latency_, _Drop Packet Count_. This is the default.

**\--full-runtime-results**
:    Periodically output a full set of results during test execution.

**\--full-final-results**
:    Output a full set of results upon test completion.

**\--result-units** _unit_
:    Display results in units _unit_. Supported values are: _B_, _K_, _M_, _G_. If no option is specified, **spiperf** will choose a reasonable default per value to minimize the number of digits in the output value.


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

This example shows an enhanced traffic configuration. This assumes the **spiperf** server is running on host _a.b.c.d_ port _4321_. It also assumes that the client's _OpenPerf_ instance is running on the same host using the default API port and that there's exactly one test port resource configured. Server configuration is omitted as it's not different from the example in the quick start section. Test will run 200Mbps of traffic downstream (server->client) and 20Mbps upstream for 2 minutes, and send a repeating frame size sequence of 64, 256, 128 bytes.

```
spiperf -c a.b.c.d -p 4321 -d 200M -u 20M -t 120 --imix acb
```

This example shows how to configure custom protocol headers. This assumes **spiperf** server instance is running on host _a.b.c.d_ port _4321_. It also assumes that the client's _OpenPerf_ instance is running on the same host using the default API port and that there's exactly one test port resource configured. Server configuration is omitted as it's not different from the example in the quick start section. Following table lists out protocols and addresses used. Arguments are on separate lines to better associate the address values with the intended header.

| Header   | Client Address    | Server Address    |
|----------|-------------------|-------------------|
| Ethernet | AA:BB:CC:DD:EE:FF | 00:11:22:33:44:55 |
| VLAN     | 17                | 200               |
| IPv4     | 192.168.1.24      | 10.0.0.1          |

```
spiperf -c a.b.c.d -p 4321 \
--headers ethernet,vlan,ipv4 \
--client-addrs aa:bb:cc:dd:ee:ff,17,192.168.1.24 \
--server-addrs 00:11:22:33:44:55,200,10.0.0.1
```

This example demonstrates how to specify _OpenPerf_ test port resources. The server is being configured to use the _OpenPerf_ instance on host _4.3.2.1_, port _6543_, and test port resource _test-port-one_. The server is being run on host _1.2.3.4_ and is listening on the default port. The client is being configured to use the _OpenPerf_ instance on the local host at the default port and using test port resource _port-two_.

```
spiperf -s -o 4.3.2.1:6543/test-port-one

spiperf -c 1.2.3.4 -o /port-two
```

# RESULTS

**spiperf** surfaces a rich subset of _OpenPerf_'s results and statistics. One important distinction between **spiperf** and _iperf3_ relates to runtime statistics. Runtime statistics in **spiperf** are always test-to-time; they measure the test from beginning until the snapshot is taken. In contrast _iperf3_ runtime statistics are for the previous measurement period.

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

Drop Count
:    Number of packets lost (e.g. transmitted but not received.) _OpenPerf_ leverages the signature field included with each packet to determine this value at runtime. A large value for this statistic can be indicative of a network that's over capacity or misconfigured. Other possible culprits include: poor connections leading to packet corruption, or even network device issues.

Out-of-Order Count
:    Number of packets that were received out of order (e.g. received in a different order than transmitted.) Packet reordering causes interruptions and reduced performance of network protocols.

With such a long list of possible results, **spiperf** allows users to select between three subsets of these values.

| Terse             | _to be named_       | Full                 |
|-------------------|---------------------|----------------------|
| Transmit Bit Rate | Transmit Bit Rate   | Transmit Bit Rate    |
| Receive Bit Rate  | Receive Bit Rate    | Receive Bit Rate     |
|                   | Transmit Byte Count | Transmit Byte Count  |
|                   | Receive Byte Count  | Receive Byte Count   |
|                   |                     | Minimum Latency      |
| Average Latency   | Average Latency     | Average Latency      |
|                   | Maximum Latency     | Maximum Latency      |
|                   | Average Jitter      | Average Jitter       |
|                   | Maximum Jitter      | Maximum Jitter       |
| Drop Cout         | Drop Count          | Drop Count           |
|                   | Out-of-Order Count  | Out-of-Order Count   |
|                   |                     | Transmit Frame Rate  |
|                   |                     | Receive Frame Rate   |
|                   |                     | Transmit Frame Count |
|                   |                     | Receive Frame Count  |
|                   |                     |                      |


# SEE ALSO

openperf(1)

