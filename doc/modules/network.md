# Network Module

The **Network** module is the part of OpenPerf core, handling network I/O load generation.

## Network server

Component of Openperf core, listens for network generator connections and supports network I/O load generation. Network server is controlled by generators by Firehose protocol.

### Network server api model

```json
{
    "id": "c95d7d96-84ce-4011-4d62-761e4a89d91c",
    "interface": "server-v4",
    "port": 3357,
    "protocol": "tcp",
    "stats": {
      "bytes_received": 0,
      "bytes_sent": 0,
      "connections": 0,
      "closed": 0,
      "errors": 0
    }
}
```

* **id** - unique network server identifier. Leave field empty to generate unique identifier.
* **interface** - bind server socket to a particular device, specified as interface name (required for *dpdk* driver)
* **port** - UDP/TCP port on which to listen for client connections
* **protocol** - protocol which is used to establish client connections
    * **tcp**
    * **udp**
* **stats** - network server I/O load statistics
    * **bytes_received** - the number of bytes read
    * **bytes_sent** - the number of bytes written
    * **connections** - the number of accepted client connections
    * **closed** - the number of closed client connections
    * **errors** - the number of errors observed during connection

## Network generator

Component of Openperf core, providing an ability for network I/O load generation.

### Network generator api model

```json
{
    "id": "47e8884a-72ad-42e7-52ae-26779ec7715a",
    "running": false,
    "config": {
        "connections": 10,
        "ops_per_connection": 10,
        "reads_per_sec": 10,
        "read_size": 65536,
        "writes_per_sec": 10,
        "write_size": 65536,
        "target": {
            "host": "127.0.0.1",
            "interface": "client-v4",
            "port": 3357,
            "protocol": "tcp"
        }
    }
}
```

* **id** - unique network generator identifier. Leave field empty to generate unique generator identifier.
* **running** - indicates whether this generator is currently running.
* **config** - configuration of network I/O load generation.
    * **connections** - number of connections to establish with the server
    * **ops_per_connection** - number of operations over a connection before closed
    * **read_size** - number of bytes to use for each read operation.
    * **reads_per_sec** - number of read operations to perform per second.
    * **write_size** - number of bytes to use for each write operation.
    * **writes_per_sec** - number of write operations to perform per second.
    * **target** - connection configuration
        * **host** - remote host to establish a connection
        * **port** - port on which client is trying to establish connection
        * **interface** - bind client socket to a particular device, specified as interface name
        * **protocol** - protocol to establish connection between client and host
            * **tcp**
            * **udp**

Leave *reads_per_sec* or *writes_per_sec* as zero value to ignore operation while I/O load generation.
Target interface is optional parameter. Required for *dpdk* driver only

### Start network generator

Start network generator command returns Network Generator Result, created for the particular start. Response *Location* header contains uri of created result.

## Network generator result

Network I/O load generation statistics. Generator creates new *Result* with new unique ID on each Start request. While load generation is running, the result has *active* state and is being updated after each generation iteration. After the generator has completed load generation the result receives final statistics values and become inactive.

### Network generator result api model

```json
{
    "id": "f1ce4985-b7fe-4efc-443b-152fc0a714fa",
    "generator_id": "47e8884a-72ad-42e7-52ae-26779ec7715a",
    "active": true,
    "timestamp_first": "2020-12-25T12:22:18.891662Z",
    "timestamp_last": "2020-12-25T12:22:20.283877Z",
    "read": {
        "bytes_actual": 655360,
        "bytes_target": 655360,
        "io_errors": 0,
        "latency_total": 55418732,
        "latency_max": 55418732,
        "latency_min": 55418732,
        "ops_actual": 10,
        "ops_target": 10
    },
    "write": {
        "bytes_actual": 655360,
        "bytes_target": 655360,
        "io_errors": 0,
        "latency_total": 55418732,
        "latency_max": 55418732,
        "latency_min": 55418732,
        "ops_actual": 10,
        "ops_target": 10
    },
    "connection": {
        "attempted": 10,
        "successful": 10,
        "closed": 0,
        "errors": 0
    }
}
```

* **id** - unique network generator result identifier.
* **generator_id** - network generator identifier that generated this result.
* **active** - indicates whether the result is currently being updated.
* **timestamp_first** - timestamp when the related generator has been started, in ISO 8601 format.
* **timestamp_last** - the ISO8601-formatted date of the last result update.
* **read** - read network I/O operation statistics.
* **write** - write network I/O operation statistics.
    * **bytes_actual** - the actual number of bytes read or written.
    * **bytes_target** - the intended number of bytes read or written.
    * **io_errors** - the number of io_errors observed during reading or writing.
    * **latency_total** - the total amount of time required to perform all operations (in nanoseconds).
    * **latency_max** - the maximum observed latency value (in nanoseconds). Field is absent if latency was not measured yet.
    * **latency_min** - the minimum observed latency value (in nanoseconds).Field is absent if latency was not measured yet.
    * **ops_actual** - the actual number of operations performed.
    * **ops_target** - the intended number of operations performed.
* **connection** - connection statistics
    * **attempted** - the number of attempts to establish connections
    * **successful** - the actual number of established connections
    * **closed** - the number of closed connections
    * **errors** - the number of errors observed during connecting process

## Commands flow

### Create Network Server

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/network/servers' \
--header 'Content-Type: application/json' \
--data-raw '{
    "id": "c95d7d96-84ce-4011-4d62-761e4a89d91c",
    "interface": "server-v4",
    "port": 3357,
    "protocol": "tcp"
}'
```

Response:
```
< HTTP/1.1 201 Created
< Location: http://<OPENPERF_HOST>:<OPENPERF_PORT>/network/servers/c95d7d96-84ce-4011-4d62-761e4a89d91c
< Content-Type: application/json
< Content-Length: 124
{
    "id": "c95d7d96-84ce-4011-4d62-761e4a89d91c",
    "interface": "server-v4",
    "port": 3357,
    "protocol": "tcp",
    "stats": {
        "bytes_received": 0,
        "bytes_sent": 0,
        "closed": 0,
        "connections": 0,
        "errors": 0
    }
}
```

### Create Network Generator
```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/network/generators' \
--header 'Content-Type: application/json' \
--data-raw '{
    "config": {
        "connections": 10,
        "ops_per_connection": 10,
        "reads_per_sec": 10,
        "read_size": 65536,
        "writes_per_sec": 10,
        "write_size": 65536,
        "target": {
            "host": "127.0.0.1",
            "interface": "client-v4",
            "port": 3357,
            "protocol": "tcp"
        }
    }
}'
```

Response:
```
< HTTP/1.1 201 Created
< Location: http://<OPENPERF_HOST>:<OPENPERF_PORT>/network/generators/47e8884a-72ad-42e7-52ae-26779ec7715a
< Content-Type: application/json
< Content-Length: 230
{
    "config": {
        "connections": 10,
        "ops_per_connection": 10,
        "reads_per_sec": 10,
        "read_size": 65536,
        "writes_per_sec": 10,
        "write_size": 65536,
        "target": {
            "host": "127.0.0.1",
            "interface": "client-v4",
            "port": 3357,
            "protocol": "tcp"
        }
    },
    "id": "47e8884a-72ad-42e7-52ae-26779ec7715a",
    "running": false
}
```

### Start existing Network Generator

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/network/generators/47e8884a-72ad-42e7-52ae-26779ec7715a/start'
```

Response:
```
< HTTP/1.1 201 Created
< Location: http://<OPENPERF_HOST>:<OPENPERF_PORT>/network/generators/e57815f4-a56c-4286-5799-bace8f1125f7/start/47e8884a-72ad-42e7-52ae-26779ec7715a
< Content-Type: application/json
< Content-Length: 649
{
    "active":true,
    "connection": {
        "attempted": 0,
        "closed": 0,
        "errors": 0,
        "successful": 0
    },
    "generator_id":"47e8884a-72ad-42e7-52ae-26779ec7715a",
    "id":"f1ce4985-b7fe-4efc-443b-152fc0a714fa",
    "read": {
        "bytes_actual":0,
        "bytes_target":0,
        "io_errors":0,
        "latency_total":0,
        "ops_actual":0,
        "ops_target":0
    },
    "timestamp_last":"2020-12-25T12:22:18.891662Z",
    "timestamp_first": "2020-12-25T12:22:18.891662Z",
    "write": {
        "bytes_actual":0,
        "bytes_target":0,
        "io_errors":0,
        "latency_total":0,
        "ops_actual":0,
        "ops_target":0
    }
}
```

### Stop Network Generator

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/network/generators/47e8884a-72ad-42e7-52ae-26779ec7715a/stop'
```

Response:
```
< HTTP/1.1 204 No Content
< Content-Type: application/json
< Content-Length: 0
```

### Check the created Network Generator Result

```bash
curl --location --request GET '<OPENPERF_HOST>:<OPENPERF_PORT>/network/generator-results/f1ce4985-b7fe-4efc-443b-152fc0a714fa'
```

Response:
```
< HTTP/1.1 200 OK
< Content-Type: application/json
< Content-Length: 649
{
    "active": false,
    "connection": {
        "attempted": 10,
        "closed": 0,
        "errors": 0,
        "successful": 10
    },
    "generator_id":"47e8884a-72ad-42e7-52ae-26779ec7715a",
    "id":"f1ce4985-b7fe-4efc-443b-152fc0a714fa",
    "read": {
        "bytes_actual": 655360,
        "bytes_target": 655360,
        "io_errors": 0,
        "latency_total": 55418732,
        "latency_max": 55418732,
        "latency_min": 55418732,
        "ops_actual": 10,
        "ops_target": 10
    },
    "timestamp_last": "2020-12-25T12:22:20.283877Z",
    "timestamp_first": "2020-12-25T12:22:18.891662Z",
    "write": {
        "bytes_actual": 655360,
        "bytes_target": 655360,
        "io_errors": 0,
        "latency_total": 55418732,
        "latency_max": 55418732,
        "latency_min": 55418732,
        "ops_actual": 10,
        "ops_target": 10
    }
}
```

## Dynamic Results field names

### For *read* statistics:
* **read.bytes_actual** - the actual number of bytes read.
* **read.bytes_target** - the intended number of bytes read.
* **read.io_errors** - the number of io_errors observed during reading.
* **read.latency_total** - the total amount of time required to perform all read operations (in nanoseconds).
* **read.latency_max** - the maximum observed latency value (in nanoseconds).
* **read.latency_min** - the minimum observed latency value (in nanoseconds).
* **read.ops_actual** - the actual number of read operations performed.
* **read.ops_target** - the intended number of read operations performed.

### For *write* statistics:
* **write.bytes_actual** - the actual number of bytes written.
* **write.bytes_target** - the intended number of bytes written.
* **write.io_errors** - the number of io_errors observed during writing.
* **write.latency_total** - the total amount of time required to perform all write operations (in nanoseconds).
* **write.latency_max** - the maximum observed latency value (in nanoseconds).
* **write.latency_min** - the minimum observed latency value (in nanoseconds).
* **write.ops_actual** - the actual number of wirite operations performed.
* **write.ops_target** - the intended number of write operations performed.

### For *connection* statistics:
* **connection.attempted** - the number of attempts to establish connections
* **connection.successful** - the actual number of established connections
* **connection.closed** - the number of closed connections
* **connection.errors** - the number of errors observed during connecting process
