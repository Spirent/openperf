# Block Module

The **Block** module is the part of OpenPerf core, handling block devices I/O load generation.

## Block device

Hardware- or pseudo- device, providing an ability for block I/O load generation.

### Block device api model

```json
{
    "id": "83b1ab46-e661-4725-6b91-2318bdbe2b34",
    "info": "",
    "path": "/dev/sdc",
    "size": 63999836160,
    "usable": true,
    "state": "init",
    "init_percent_complete": 33
}
```

* **id** - automatically generated unique device identificator. Unique for each openperf launch.
* **info** - any info related to the current block device.
* **path** - resource pathname.
* **size** - resource size (in bytes).
* **usable** - indicates whether it is safe to use this device for block I/O load generation.
* **state** - initialization status of the block device.
    * **uninitialized** - device is not initialized.
    * **initializing** - device initialization is in progress state.
    * **ready** - device is ready for I/O load generation.
* **init_percent_complete** - percentage of initialization completed so far.

## Block file

An interface for a block device, providing an ability for block I/O load generation with specifing file system path and maximum size, limiting block I/O load generation.
If the file does not exist or its size differs from the size provided in the create API command, the file will be resized and filled with generated pseudo random data.

### Block file api model

```json
{
    "id": "9f6ef1af-060b-47a8-7f21-b72cf0495aaf",
    "init_percent_complete": 3,
    "file_size": 2147483648,
    "path": "/tmp/foo",
    "state": "init"
}
```

* **id** - unique file identificator. Leave field empty to generate unique file identificator.
* **init_percent_complete** - percentage of initialization completed so far.
* **file_size** - size of test file (in bytes).
* **path** - resource pathname.
* **state** - initialization status of the block file.
    * **none** - file initialization is in failed state.
    * **init** - file initialization is in progress state.
    * **ready** - file is ready for I/O load generation.

## Block generator

Component of Openperf core, providing an ability for block I/O load generation. Requires block resource such as Block File or Block Device.

### Block generator api model

```json
{
    "config": {
        "pattern": "random",
        "queue_depth": 10,
        "read_size": 65536,
        "reads_per_sec": 40,
        "write_size": 65536,
        "writes_per_sec": 10
    },
    "id": "3f79937d-53ce-4411-435c-3139ea4a954c",
    "resource_id": "9f6ef1af-060b-47a8-7f21-b72cf0495aaf",
    "running": false
}
```

* **id** - unique block generator identifier. Leave field empty to generate unique file identificator.
* **resource_id** - unique device or file identifier chosen for I/O load generation.
* **running** - indicates whether this generator is currently running.
* **config** - configuration of block I/O load generation.
    * **queue_depth** - maximum number of simultaneous (asynchronous) operations.
    * **read_size** - number of bytes to use for each read operation.
    * **reads_per_sec** - number of read operations to perform per second.
    * **write_size** - number of bytes to use for each write operation.
    * **writes_per_sec** - number of write operations to perform per second.
    * **pattern** - I/O access pattern.
        * **random** - access to the random point of the resource.
        * **sequential** - sequential access to the resource from the beginning to the end.
        * **reverse** - sequential reverse access to the resource from the end to the beginning.

Leave *reads_per_sec* or *writes_per_sec* as zero value to ignore operation while I/O load generation.

### Start block generator

Start block generator command returns Block Generator Result, created for the particular start. Response *Location* header contains uri of created result.

## Block generator result

Block I/O load generation statistics. Generator creates new *Result* with new unique ID on each Start request. While load generation is running, the result has *active* state and is being updated after each generation iteration. After the generator has completed load generation the result receives final statistics values and become inactive.
Active generator result cannot be deleted.

### Block generator result api model

```json
{
    "active": true,
    "generator_id": "3f79937d-53ce-4411-435c-3139ea4a954c",
    "id": "24554373-e8df-4ad5-582c-21030e1d4cce",
    "read": {
        "bytes_actual": 80,
        "bytes_target": 80,
        "io_errors": 0,
        "latency_total": 12652,
        "latency_max": 12652,
        "latency_min": 12652,
        "ops_actual": 10,
        "ops_target": 10
    },
    "timestamp": "1970-01-13T00:48:40.283877Z",
    "write": {
        "bytes_actual": 0,
        "bytes_target": 0,
        "io_errors": 0,
        "latency_total": 0,
        "ops_actual": 0,
        "ops_target": 0
    }
}
```

* **id** - unique block generator result identifier.
* **generator_id** - block generator identifier that generated this result.
* **active** - indicates whether the result is currently being updated.
* **timestamp** - the ISO8601-formatted date of the last result update.
* **read** - read block I/O operation statistics.
* **write** - write block I/O operation statistics.
    * **bytes_actual** - the actual number of bytes read or written.
    * **bytes_target** - the intended number of bytes read or written.
    * **io_errors** - the number of io_errors observed during reading or writing.
    * **latency_total** - the total amount of time required to perform all operations (in nanoseconds).
    * **latency_max** - the maximum observed latency value (in nanoseconds). Field is absent if latency was not measured yet.
    * **latency_min** - the minimum observed latency value (in nanoseconds).Field is absent if latency was not measured yet.
    * **ops_actual** - the actual number of operations performed.
    * **ops_target** - the intended number of operations performed.

## Commands flow

### Create Block File or use Block Device instead

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/block-files' \
--header 'Content-Type: application/json' \
--data-raw '{
    "file_size": 2147483648,
    "path": "/tmp/foo"
}'
```

Response:
```
< HTTP/1.1 201 Created
< Location: http://<OPENPERF_HOST>:<OPENPERF_PORT>/block-files/96a41125-3ecc-40f0-5833-5dccf6383dcd
< Content-Type: application/json
< Content-Length: 124
{
    "file_size":2147483648,
    "id":"96a41125-3ecc-40f0-5833-5dccf6383dcd",
    "init_percent_complete":0,
    "path":"/tmp/foo",
    "state":"init"
}
```

### Create Block Generator
```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/block-generators' \
--header 'Content-Type: application/json' \
--data-raw '{
  "config": {
    "queue_depth": 128,
    "reads_per_sec": 100,
    "read_size": 10485,
    "writes_per_sec": 100,
    "write_size": 10485,
    "pattern": "random"
  },
  "resource_id": "96a41125-3ecc-40f0-5833-5dccf6383dcd",
  "running": false
}'
```

Response:
```
< HTTP/1.1 201 Created
< Location: http://<OPENPERF_HOST>:<OPENPERF_PORT>/block-generators/e57815f4-a56c-4286-5799-bace8f1125f7
< Content-Type: application/json
< Content-Length: 230
{
    "config": {
        "pattern":"random",
        "queue_depth":128,
        "read_size":10485,
        "reads_per_sec":100,
        "write_size":10485,
        "writes_per_sec":100
    },
    "id":"e57815f4-a56c-4286-5799-bace8f1125f7",
    "resource_id":"96a41125-3ecc-40f0-5833-5dccf6383dcd",
    "running":false
}
```

### Start existing Block Generator

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/block-generators/e57815f4-a56c-4286-5799-bace8f1125f7/start'
```

Response:
```
< HTTP/1.1 201 Created
< Location: http://<OPENPERF_HOST>:<OPENPERF_PORT>/block-generators/e57815f4-a56c-4286-5799-bace8f1125f7/start/3d03c5d7-6a5a-45d6-7711-5c8e7e26f66d
< Content-Type: application/json
< Content-Length: 419
{
    "active":true,
    "generator_id":"e57815f4-a56c-4286-5799-bace8f1125f7",
    "id":"8d3d25b6-b02c-48c4-5eb5-74cfcd3540cb",
    "read": {
        "bytes_actual":0,
        "bytes_target":0,
        "io_errors":0,
        "latency_total":0,
        "ops_actual":0,
        "ops_target":0
    },
    "timestamp":"1970-01-14T08:46:14.183615Z",
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

### Stop Block Generator

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/block-generators/e57815f4-a56c-4286-5799-bace8f1125f7/stop'
```

Response:
```
< HTTP/1.1 204 No Content
< Content-Type: application/json
< Content-Length: 0
```

### Check the created Block Generator Result

```bash
curl --location --request GET '<OPENPERF_HOST>:<OPENPERF_PORT>/block-generator-results/3d03c5d7-6a5a-45d6-7711-5c8e7e26f66d'
```

Response:
```
< HTTP/1.1 200 OK
< Content-Type: application/json
< Content-Length: 483
{
    "active": false,
    "generator_id": "e57815f4-a56c-4286-5799-bace8f1125f7",
    "id": "8d3d25b6-b02c-48c4-5eb5-74cfcd3540cb",
    "read": {
        "bytes_actual": 2862405,
        "bytes_target": 2862405,
        "io_errors": 0,
        "latency_total": 78215989,
        "latency_max": 3665760,
        "latency_min": 9887,
        "ops_actual": 546,
        "ops_target": 546
    },
    "timestamp": "2020-04-30T08:59:40.301996Z",
    "write": {
        "bytes_actual": 2862405,
        "bytes_target": 2862405,
        "io_errors": 0,
        "latency_total": 47406175,
        "latency_max": 1135615,
        "latency_min": 24579,
        "ops_actual": 546,
        "ops_target": 546
    }
}
```
