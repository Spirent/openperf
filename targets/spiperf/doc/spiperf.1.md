% SPIPERF(1)
% Spirent Communications
% November 2019

# NAME

spiperf - test network throughput, latency, and loss.

# SYNOPSIS

**spiperf** [**-h**]

# DESCRIPTION

**spiperf** is a frontend for OpenPerf in the style of `iperf3`. It configures traffic between two network devices and provides statistics related to throughput, latency, and loss, if any. An emphasis has been placed on ease-of-use to allow minimal user input to run a meaningful test. At the same time a rich assortment of command-line options are provided so that discerning users may have enhanced control over their tests.

**spiperf** performs its tests by generating stateless network traffic in one or both directions between two network devices. Using a special signature embedded within each packet it is able to determine latency and loss in one direction without the need for round-trip transmission.

A single **spiperf** instance controls two _openperf_ instances, one on each network device. One of these instances is referred to as the `client` and the other as the `server`. This distinction is used purely for descriptive purposes. In practice the only difference is **spiperf** defaults to connecting to the `client` instance on the same network device. This terminology is borrowed from `iperf`.

# QUICK START

The simplest **spiperf** test can be run with a single command:

```
spiperf -S <_server ip_>
```

This will result in traffic being sent at 10Mbps for 10 seconds from the local _openperf_ `client` instance to the `server` one at <_server ip_>.

Behind the scenes **spiperf** will select the first _port_ resource of each _openperf_ instance and provision the traffic test between them. **spiperf** will then run traffic and collate statistics from client and server instances.

# OPTIONS SUMMARY

| option                       | description                                          |
| :--------------------------- | :--------------------------------------------------- |
| **-h**, **\--help**          | display help text                                    |
| **-v**, **\--verbose**       | verbose output                                       |
| **-q**, **\--quiet**         | quieter output                                       |
| **-d**, **\--downstream**    | send traffic from server to client at _rate_         |
| **-u**, **\--upstream**      | send traffic from client to server at _rate_         |
| **-t**, **\--time**          | traffic duration time in seconds                     |
| **-l**, **\--packet-length** | fixed packet length                                  |
| **\--imix**                  | send multiple packet sizes                           |
| **\--fill-pattern**          | byte pattern to fill packets with                    |
| **-C**, **\--client-api**    | client OpenPerf instance location                    |
| **-S**, **\--server-api**    | server OpenPerf instance location                    |
| **-c**, **\--client-port**   | client port for test traffic                         |
| **-s**, **\--server-port**   | server port for test traffic                         |
| **-f**, **\--file**          | output results to a file                             |
| **-i**, **\--interval**      | interval between runtime result output               |
| **\--headers**               | list of protocol headers to use in test traffic      |
| **\--client-addrs**          | list of client-side addresses to use in test traffic |
| **\--server-addrs**          | list of server-side addresses to use in test traffic |
| **\--version**               | output program version                               |


# OPTIONS

**-h**, **\--help**
:    Display usage summary.

**-v**, **\--verbose**
:    Increase verbosity of output. Adding additional **-v** further increases verbosity.

**-q**, **\--quiet**
:    Quiet output. Only show end-of-test result. Adding additional **-q** quiets output further. 

**-d**, **\--downstream** [_rate_]
:    Send traffic from _server_ to _client_. Optional rate parameter is in _bps_ and supports _K_, _M_, _G_ suffix.

**-u**, **\--upstream** [_rate_]
:    Send traffic from _client_ to _server_. Optional rate parameter is in _bps_ and supports _K_, _M_, _G_ suffix.

**-t**, **\--time** [_time_]
:    Time in seconds to run traffic for. Defaults to 10 seconds.

**-l**, **\--packet-length** [_length_]
:    Fixed packet length to send in bytes. Defaults to 128 bytes.

**\--imix** 
:    Use _IMIX_ frame length distribution. Mutually exclusive with **-l**.
 
**\--fill-pattern** [_bytes_] | [**prbs**]
:    Pattern of bytes to fill payload portion. Must be in the format 0xYYYY where Y is a hex digit. Special value _prbs_ can be supplied to use prbs fill.

**-C**, **\--client-api** [_server ip_]_:port_
:    **openperfd** instance to use for client side. If omitted **spiperf** will attempt to connect to an instance running at the default port on the local host.

**-S**, **\--server-api** [_server ip_]_:port_
:    **openperfd** instance to use for server side.

**-c**, **\--client-port** [_identifier_]
:    **openperfd** port identifier to use for test traffic. If omitted **spiperf** will use the sole port resource of the client **openperf** instance.

**-s**, **\--server-port** [_identifier_]
:    OpenPerf port identifier for test traffic. If omitted **spiperf** will use the sole port resource of the server **openperf** instance.

**-f**, **\--file** _file_
:    Output periodic and end-of-test results to file indicated by _file_. If it does not exist **spiperf** will create it. If it does exist, append to it.

**-i**, **\--interval**
:    Interval in seconds between outputting periodic statistics. Defaults to 1.

**\--headers** _header_[,_header_]
:    Comma-separated list of protocol headers to use in test traffic packets. List is processed left-to-right and applied to the packet in that order. Defaults to _Ethernet_,_IPv4_,_UDP_. See **--help** output for a full list of supported headers. Case-insensitive. A non-exhaustive list includes: _Ethernet_, _VLAN_, _IPv4_, _IPv6_, _UDP_. Not all permutations are supported.

**\--client-addrs** _address_[,_address_]
:    Comma-separated list of client-side addresses to use in test traffic packets. List is processed left-to-right and applied to the packet in that order. See **--help** output for a full list of supported headers. A special value of _auto_ can be supplied in place of a specific address and **spiperf** will auto-generate that address.

**\--server-addrs** _address_[,_address_]
:    Comma-separated list of server-side addresses to use in test traffic packets. List is processed left-to-right and applied to the packet in that order. See **--help** output for a full list of supported headers. A special value of _auto_ can be supplied in place of a specific address and **spiperf** will auto-generate that address.

**\--version**
:    Output program version.

# EXIT STATUS

Need this?

# EXAMPLES

Example of enhanced traffic configuration. This assumes server side _openperf_ instance is running on host _a.b.c.d_ port _4321_ and the client instance is running on the local host with the default API port. Test will run 200Mbps of traffic downstream (server->client) and 20Mbps upstream for 2 minutes, use multiple frame sizes, and save results to _results.txt_.

```
spiperf -S a.b.c.d:4321 -d 200M -u 20M -t 120 --imix -f results.txt
```

Example showing how to configure custom protocol headers. This assumes server side _openperf_ instance is running on host _a.b.c.d_ port _4321_ and there's a local client _openperf_ instance on port _4567_. Following table lists out protocols and addresses used. Arguments are on separate lines to better associate the address values with the intended header.

| Header   | Client Address    | Server Address    |
|----------|-------------------|-------------------|
| Ethernet | AA:BB:CC:DD:EE:FF | 00:11:22:33:44:55 |
| VLAN     | 17                | 200               |
| IPv4     | 192.168.1.24      | 10.0.0.1          |
| UDP      | 6119              | 2420              |


```
spiperf -C :4567 -S a.b.c.d:4321 \
--headers ethernet,vlan,ipv4,udp \
--client-addrs aa:bb:cc:dd:ee:ff,17,192.168.1.24,6119 \
--server-addrs 00:11:22:33:44:55,200,10.0.0.1,2420
```
# SEE ALSO

openperfd(1)


# Questions and Comments for reviewers
1. OpenPerf vs Openperf vs openperfd?
2. Options sections grouped by function rather than straight alpha order.

