# OpenPerf Getting Started Guide

This guide documents the steps necessary to use OpenPerf as a user-space
stack for 3rd part applications, such as `iperf`.

## Prerequisites

Currently, OpenPerf only runs on `Linux`.  Additionally, your host
platform will need at least three CPU cores and one DPDK compatible NIC.
As `openperf` is a DPDK based application, you will need to make sure
your target platform can support a DPDK program.  Consult the relevant
[DPDK system requirements](https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html)
for details.

Once you have a DPDK compatible platform, you will need to build the OpenPerf
targets.

### Targets

You will need to build two targets: `openperf` and `libopenperf-shim.so`.

#### OpenPerf

The `openperf` binary contains a REST API server and a user-space stack.
To build, type `make` in the `targets/openperf` directory.
To run, simply `exec` the resulting binary.

The `openperf` binary provides a few command line options to control its
behavior.

- `--api-port`: specify the listening port for the REST server
- `--dpdk`: provide DPDK specific options directly; enclose options
  in parenthesis.
- `--log-level`: change the log level.  Supported log levels in order of
  increasing verbosity are:
  - **CRITICAL**: log fatal errors
  - **ERROR**: log any type of error
  - **WARNING**: log errors and unexpected conditions
  - **INFO**: log informational messages; this is the default
  - **DEBUG**: log developer details
  - **TRACE**: log everything

**Note**: OpenPerf will automatically adjust the DPDK log level based on the
`--log-level` parameter.

Finally, logs are written to `stdout` in **JSON** format.

#### libopenperf-shim.so

The `libopenperf-shim.so` library contains a BSD socket implementation for
`openperf`.
To build, type `make` in the `targets/libopenperf-shim` directory.

## Running

**Note**: Currently, `openperf` and its client processes require elevated
privileges to run. This is not desired behavior and will be addressed
soon.

To run `iperf` using `openperf`, follow these steps:

1. Start the `inceopenperfption` process. At a minimum, you may need to use the
   `--dpdk` option to specify a core mask for OpenPerf to use. To run
   `openperf` on all even numbered cores on an 8 core host, use:
   ```
   sudo <openperf build path>/bin/openperf --dpdk="-c=0x55"
   ```
2. Create an interface for the stack.  This can be done by sending a properly
   formed POST request to the REST API socket.  The snippet below creates
   an interface on port 0.
   ```javascript
   {
     "config": {
       "protocols": [
         {
           "eth": {
             "mac_address": "00:10:94:ae:d6:aa"
           }
         },
         {
           "ipv4": {
             "method": "static",
             "static": {
               "address": "198.51.100.10",
               "prefix_length": 24,
               "gateway": "198.51.100.1"
             }
           }
         }
       ]
     },
     "port_id": "0"
   }
   ```
   *Note*: The MAC address in the example above uses the Spirent OUI.
   However, the choice of MAC address partially depends on the underlying
   port type.  When using hardware NICs, you may choose any sane MAC address
   value.  Such MAC addresses _should_ be non-broadcast, non-multicast, and
   locally administered.  However, some virtual port types, e.g. `virtio` may
   need the MAC address to match the "hardware" MAC address in order to transmit
   and receive packets.  Caveat utilitor.

   You can use the `curl` program to configure the interface. Assume the
   above interface config has been written to the file `interface.json`,
   then the following command would create the interface in the stack:
   ```
   curl "http://localhost:<port>/interfaces" -d@interface.json
   {
     "config": {
       "protocols": [
         {
           "eth": {
             "mac_address": "00:10:94:ae:d6:aa"
           }
         },
         {
           "ipv4": {
             "method": "static",
             "static": {
               "address": "198.51.100.10",
               "gateway": "198.51.100.1",
               "prefix_length": 24
             }
           }
         }
       ]
     },
     "id": "0",
     "port_id": "0",
     "stats": {
       "rx_bytes": 0,
       "rx_errors": 0,
       "rx_packets": 0,
       "tx_bytes": 42,
       "tx_errors": 0,
       "tx_packets": 1
     }
   }
   ```
   A successful request returns the full interface representation (as well as a
   200 HTTP response code).
3. Run your client app using a **LD_PRELOAD** variable to access the stack
   ```
   sudo LD_PRELOAD=<openperf build path>/lib/libopenperf-shim.so \
   iperf -c <iperf server address>
   ```
   Obviously, you will need another `iperf` instance running at the specified
   IP address.
