
# Configuration

Configuration parameters can be specified in a YAML configuration file, or as command line arguments.

## Common
- `-c, --config`

  Configuration file.

- `--version`

  Print version and exit.

- `--help`

  Print help and exit.

## Core
- `-P, --core.prefix`

   Prefix for temporary file system files, e.g. sockets. Needed when running multiple instances.

- `-l, --core.log.level`

   Used to specify the log level. Currently supported log levels are:
     1. critical
     2. error
     3. warning
     4. info
     5. debug
     6. trace

## Modules

### Plugins

- `-m, --modules.plugins.path`

  Specifies path to plugin modules. This path will be used to searching the modules specified by `modules.plugins.load` option.

- `-L, --modules.plugins.load`

  Comma separated plugin module file names.

### API
- `-p, --modules.api.port`

  Rest API HTTP port.

### Block
- `--modules.block.cpu-mask`

  Specifies a CPU core affinity mask for all Block module threads, in hex. By, default, block module will use all available CPU's.

### CPU
- `--modules.cpu.cpu-mask`

  Specifies a CPU core affinity mask for all CPU module threads, in hex. By, default, cpu module will use all available CPU's.

### Memory
- `--modules.memory.cpu-mask`

  Specifies a CPU core affinity mask for all Memory module threads, in hex. By, default, memory module will use all available CPU's.

### Socket
- `-f, --modules.socket.force-unlink`

  Force removal of stale files.

### PacketIO
- `--modules.packetio.cpu-mask`

  Provide an explicit CPU mask for packetio module threads. By, default, packetio will use all available CPU's. Setting this option to 0 will disable the packetio module.

### DPDK
- `--modules.packetio.dpdk.no-rx-interrupts`

  Use polling instead of interrupts to service DPDK receive queues.

  *Note: polling will always be used for drivers that do not support receive interrupts.*

- `--modules.packetio.dpdk.no-lro`

  Disable Large Receive Offload. LRO is used by default if supported by the driver; unfortunately, many drivers are buggy.

- `--modules.packetio.dpdk.no-init`

  Disable DPDK entirely. The PacketIO module and all dependent modules will be unusable. This is equivalent to setting the packetio CPU mask to 0.

- `-d, --modules.packetio.dpdk.options`

  Provide explicit options to DPDK.

  *Example:* "-m256m,--no-huge"

- `--modules.packetio.dpdk.port-ids`

  Use well-known strings for port id's instead of random UUID's. This option is necessary when creating port based resources in the configuration file.

  *Example:* "port0:port0,port1:port1"

- `--modules.packetio.dpdk.test-mode`

  Enable test mode by creating loopback port pairs.

- `--modules.packetio.dpdk.test-portpairs`

  Number of loopback port pairs for testing, defaults to 1.

- `-M, --modules.packetio.dpdk.misc-worker-mask`

  Provide an explicit mask for miscellaneous worker threads. Must be a subset of the module or DPDK mask. Currently only used for stack threads. Setting this mask to 0x0 will disable the stack.

- `-R, --modules.packetio.dpdk.rx-worker-mask`

  Provide an explicit mask for receive worker threads. Must be a subset of the module or DPDK mask.

- `-T, --modules.packetio.dpdk.tx-worker-mask`

  Provide an explicit mask for transmit worker threads. Must be a subset of the module or DPDK mask.

## Configuration file

All structured long options, starting with `--` and containing dot delimiter can be specified in a configuration YAML file. Comma separated lists can be converted to an array in YAML configuration file. For example:

```YAML
core:
  log:
    level: warning

modules:
  api:
    port: 9000
  plugins:
    path: "/usr/lib/openperf/plugins"
```

## Configuration for AAT

When running AAT, the typical configuration is

```YAML
modules:
  packetio:
    dpdk:
      options:
        - "-m256m"
        - "--no-huge"
      test-mode: true
      port-ids:
        port0: port0
        port1: port1

resources:
  /interfaces/dataplane-server:
    port_id: port0
    ..

  /interfaces/dataplane-client:
    port_id: port1
    ...

```

## resources initialization

During the initialization, the API module sends a _internal_ HTTP post to itself to create the _interfaces_ resources, which is then sent to the Packet IO module via 0MQ.

Let consider the following interface need to be created:

```YAML
  /interfaces/interface-one:
    port_id: port0
    config:
      protocols:
        - eth:
            mac_address: "00:10:94:ae:d6:aa"
        - ipv4:
            method: static
            static:
              address: "198.51.100.10"
              prefix_length: 24
              gateway: "198.51.100.1"
```

This configuration will be stored using the following type:

```C++
struct config_data {
    std::vector<protocol_config> protocols;
    std::string port_id;
    std::string id;
};

typedef std::variant<std::monostate, eth_protocol_config, ipv4_protocol_config> protocol_config;

...
```

The resource can also contain ports, eg:

```YAML
resources:
  /ports/port-bond:
    kind: bond
    config:
      bond:
        mode: lag_802_3_ad
        ports:
          - port0
          - port1
```

This will create a new port (`port-bond`), and the bounding is done using DPDK `rte_eth_bond_create` method. Note that only `802.3AD` lad mode is supported.
