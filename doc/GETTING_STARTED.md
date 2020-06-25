# OpenPerf Getting Started Guide

This guide documents the steps necessary to use OpenPerf as a user-space
stack for 3rd party applications, such as `iperf`.

## Overview

Using OpenPerf requires two distinct components: a server and clients.

### Server

The server process, `openperf`, initializes the user-space stack and
connects it to network interface cards (NICs) using DPDK libraries.
This process also provides a REST API interface over the host's network
stack to allow configuration and querying of OpenPerf's stack and NICs.

### Clients

Client can be any BSD socket based application. In this guide, we will
use `iperf` as our client. Clients access the server process by using
the `LD_PRELOAD` environment variable to load a shim library that
redirects BSD socket calls to OpenPerf.

## Prerequisites

OpenPerf only runs on `Linux`.  Additionally, your host
platform will need at least two CPU cores and one DPDK compatible NIC.
Since `openperf` is a DPDK based application, you will need to make sure
your target platform can support a DPDK program.  Consult the relevant
[DPDK system requirements](https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html)
for details.

### Permissions

While `openperf` does not require any special user permissions,
certain DPDK drivers and options do. At a minimum, the user account
running the `openperf` binary will need to have sufficient permissions
to be able to run a DPDK application.

Generally, this requires the following:
* Your user account must have sufficient permission to access the `hugetablfs`
  on `/dev/hugepages` (or wherever huge pages are mounted on your host system).
* Your user account must have sufficient permission to run the user space driver
  for the host NIC.

Note: different DPDK drivers require different capability sets! Configuring
permissions for your specific driver and host system is outside the scope of
this document.

#### Security Issues

##### sudo

Just don't. The OpenPerf server and client processes require shared
memory in order to function. Linux disallows processes with different
capability/privilege sets from sharing memory. Hence, if you run
`openperf` with root privileges, then all client processes must also
run with root privileges.

##### LD_PRELOAD

Binaries with elevated privileges **cannot** use `LD_PRELOAD` to hook system
calls. Hence, any intended client applications with explicit file capabilities,
cf. `getcap`, or with the `setgid` or `setuid` flags set will not work with
OpenPerf.

For example, the traditional network utility `ping` always requires
elevated privileges to access raw sockets and can not be used
directly. This guide shows how to work around this limitation below.

## Getting Binaries

You may either build OpenPerf on your host or download the latest build artifacts
from the project continuous integration (CI) server. In either case, you need
two build artifacts for this guide: the binary, `openperf`, and the preload
library shim, `libopenperf-shim.so`.

Note: this guide assumes you are using locally built artifacts. Be sure to
adjust your paths if using downloaded binaries!

### Building

OpenPerf code is built using `clang`, `clang++`, and `ispc` so those
tools must be present on the build system.

Additionally, OpenPerf requires both the `libnuma` and `libcap` libraries
and headers to be installed.

All other dependencies are included in the OpenPerf code repository.

Once all of the dependencies are installed, simply type `make -j
$(nproc)` in the top level of the code repository. By default, all
build files are written to the `build` directory, which will be created
if it does not exist.

### Downloading

You will need both the `libnuma` and `libcap` libraries installed on your system.

The latest binaries are available at the links below.

- [openperf](https://circleci.com/api/v1.1/project/github/Spirent/openperf/latest/artifacts/0/~/project/build/openperf-linux-x86_64-testing/bin/openperf)
- [libopenperf-shim.so](https://circleci.com/api/v1.1/project/github/Spirent/openperf/latest/artifacts/0/~/project/build/libopenperf-shim-linux-x86_64-testing/lib/libopenperf-shim.so)

Be sure to enable execute permissions on `openperf`!

## Build Artifacts

### OpenPerf

The `openperf` binary contains a REST API server and a user-space
stack.  The default build options generate this binary in
the `build/openperf-linux-x86_64-testing/bin` directory.

#### Options

The `openperf` binary provides numerous command line options to control its
behavior. The most useful options are described below.

- `--modules.api.port` or `-p`: specify the listening port for the REST server
- `--modules.packetio.dpdk.options` or `-d`: provide DPDK specific options directly; enclose options
  in parenthesis.
- `--core.log.level` or `-l`: change the log level.  Supported log levels in order of
  increasing verbosity are:
  - **CRITICAL**: log fatal errors
  - **ERROR**: log any type of error
  - **WARNING**: log errors and unexpected conditions
  - **INFO**: log informational messages; this is the default
  - **DEBUG**: log developer details
  - **TRACE**: log everything
- `--config` or `-c`: optional YAML configuration file

Note: run time configuration may be manipulated via the REST API, the YAML
configuration file, or a combination of both.

### libopenperf-shim.so

The `libopenperf-shim.so` library contains a BSD socket implementation
for OpenPerf.  The default build options place this library in the
`build/libopenperf-shim-linux-x86_64-testing/lib/` directory.

#### Options

The shim library can be configured via environment variables:

- OP_BINDTODEVICE: Specifies the specific interface the client process
  should use for network traffic. Set this variable to the configured port
  id string. This can be helpful to force client programs to use explicit
  OpenPerf interfaces.
- OP_TRACE: When this variable is set to a non-zero value, the library
  will log intercepted BSD socket calls to stderr.

## Running

To run `iperf` using `openperf`, follow these steps:

1. Generate a configuration file. The configuration file names
   the physical port we want to use. It also allows us to create
   an interface on the port and assign a network address to it.
   Here's the configuration file for this example.
   ```YAML
   core:
       log:
           # Set this to debug or trace if you're curious about OpenPerf innards
           level: info

    modules:
        packetio:
        dpdk:
            port-ids:
                # If we don't set the port id, it will be randomly generated
                port0: port0

    resources:
        # This create a 'demo' interface on the first physical port
        /interfaces/demo0:
            port_id: port0
            config:
            protocols:
                - eth:
                    mac_address: "00:10:94:f0:0b:a4"
                - ipv4:
                    method: static
                    static:
                        address: "198.18.25.1"
                        prefix_length: 15
   ```

2. Start the `openperf` process.  This example uses DPDK options to
   limit DPDK to a specific PCI devices and set of cores.  Your DPDK
   options should probably be different!

   ```
   build/openperf-linux-x86_64-testing/bin/openperf -d "-w 9:00.0 -c 0xff" -c <config file path>/config.yaml
   ```

   Tip: If you made a typo in your configuration file, you might need to append the `-f`
   option to the command line. This option tells `openperf` to clean up any stale
   state data that might be left behind from previous instances.

3. Verify the `openperf` configuration. We can use the REST API to check
   the configuration of our ports and interfaces.

   Note: by default, the REST server is listening on port 8080, however
   this can be changed via the command line or configuration file.

   To check our physical ports, we can query the `/ports` endpoint:

   ```
   curl "http://localhost:8080/ports" | jq
   ```

   And we will receive a response like this:
   ```javascript
   [
     {
       "config": {
       "dpdk": {
          "auto_negotiation": true
       }
       },
       "id": "port0",
       "kind": "dpdk",
       "stats": {
         "rx_bytes": 0,
         "rx_errors": 0,
         "rx_packets": 0,
         "tx_bytes": 60,
         "tx_errors": 0,
         "tx_packets": 1
       },
       "status": {
         "duplex": "full",
         "link": "up",
         "speed": 25000
       }
     }
   ]
   ```

   Similarly, we can check our interfaces by querying the `/interfaces` endpoint:
   ```
   curl "http://localhost:8080/interfaces" | jq
   ```
   which gives us:
   ```
   [
     {
       "config": {
         "protocols": [
           {
             "eth": {
               "mac_address": "00:10:94:c0:ff:ee"
             }
           },
           {
             "ipv4": {
               "method": "static",
               "static": {
                 "address": "198.18.25.1",
                 "prefix_length": 15
               }
             }
           }
         ]
       },
       "id": "demo0",
       "port_id": "port0",
       "stats": {
         "rx_bytes": 0,
         "rx_errors": 0,
         "rx_packets": 0,
         "tx_bytes": 42,
         "tx_errors": 0,
         "tx_packets": 1
       }
     }
   ]
   ```

4. Start the `iperf` server. The `iperf` server process can run on any
   host that can be reached from OpenPerf's interface, including the localhost.
   No special options are required, however, you must know the IP address
   of the host running the `iperf` server process in order to connect to it
   from the `iperf` client process.

   ```
   iperf3 -s
   ```

5. Verify OpenPerf connectivity to the `iperf` server.  We'll use the
   `ping` utility as an OpenPerf client to check connectivity.
   Remember that the interaction between LD_PRELOAD and processes with
   elevated privileges means that we cannot run `ping` directly. So
   first, make a non-privileged copy of the `ping` binary.

   ```
   cp $(which ping) /tmp/safe-ping
   ```

   Now, use the shim library and our `ping` copy to ping the `iperf`
   server. In this example, the server's IP address is 198.19.25.1.

   ```
   LD_PRELOAD=build/libopenperf-shim-linux-x86_64-testing/lib/libopenperf-shim.so /tmp/safe-ping -c1 198.19.25.1
   WARNING: your kernel is veeery old. No problems.
   WARNING: setsockopt(IP_RECVTTL): Protocol not available
   WARNING: setsockopt(IP_RETOPTS): Protocol not available
   PING 198.19.25.1 (198.19.25.1) 56(84) bytes of data.
   Warning: no SO_TIMESTAMP support, falling back to SIOCGSTAMP
   64 bytes from 198.19.25.1: icmp_seq=1 ttl=0 time=0.240 ms

   --- 198.19.25.1 ping statistics ---
   1 packets transmitted, 1 received, 0% packet loss, time 0ms
   rtt min/avg/max/mdev = 0.240/0.240/0.240/0.000 ms
   ```

   Success!

6. Finally, run `iperf` as an OpenPerf client. This step follows the same pattern
   as the previous one. We'll use the shim library to force `iperf` to
   use the OpenPerf stack instead of the host stack.

   ```
   LD_PRELOAD=build/libopenperf-shim-linux-x86_64-testing/lib/libopenperf-shim.so iperf3 -c 198.19.25.1
   Connecting to host 198.19.25.1, port 5201
   [  7] local 198.18.25.1 port 55213 connected to 198.19.25.1 port 5201
   [ ID] Interval           Transfer     Bitrate         Retr  Cwnd
   [  7]   0.00-1.00   sec  2.07 GBytes  17.8 Gbits/sec    0   3.09 MBytes
   [  7]   1.00-2.00   sec  1.72 GBytes  14.8 Gbits/sec    3   2.13 MBytes
   [  7]   2.00-3.00   sec  2.06 GBytes  17.7 Gbits/sec    0   3.23 MBytes
   [  7]   3.00-4.00   sec  1.97 GBytes  16.9 Gbits/sec    0   4.00 MBytes
   [  7]   4.00-5.00   sec  1.99 GBytes  17.1 Gbits/sec    0   4.66 MBytes
   [  7]   5.00-6.00   sec  2.02 GBytes  17.3 Gbits/sec    0   5.24 MBytes
   [  7]   6.00-7.00   sec  2.00 GBytes  17.2 Gbits/sec    0   5.76 MBytes
   [  7]   7.00-8.00   sec  1.94 GBytes  16.7 Gbits/sec    0   6.22 MBytes
   [  7]   8.00-9.00   sec  1.99 GBytes  17.1 Gbits/sec   11   1.09 MBytes
   [  7]   9.00-10.00  sec  1.72 GBytes  14.8 Gbits/sec    0   2.47 MBytes
   - - - - - - - - - - - - - - - - - - - - - - - - -
   [ ID] Interval           Transfer     Bitrate         Retr
   [  7]   0.00-10.00  sec  19.5 GBytes  16.7 Gbits/sec   14             sender
   [  7]   0.00-10.20  sec  19.5 GBytes  16.4 Gbits/sec                  receiver

   iperf Done.
   ```
