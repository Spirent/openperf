# Block Module

The **Block** module is the core of OpenPerf, handling block devices I/O load generation. It is defined as a `service` which runs multiple servers/stack and components.

## Block device

```json
{
    "id": "83b1ab46-e661-4725-6b91-2318bdbe2b34",
    "info": "",
    "path": "/dev/sdc",
    "size": 4095989514240,
    "usable": true
}
```

* **id** - automatically generated unique device identificator. Unique for each openperf launch
* **info** - any info related to the current block device
* **path** - resource pathname
* **size** - resource size (in bytes)
* **usable** - indicates whether it is safe to use this device for block I/O load generation

## Block file

```json
{
    "id": "9f6ef1af-060b-47a8-7f21-b72cf0495aaf",
    "init_percent_complete": 3,
    "file_size": 63999836160,
    "path": "/tmp/foo",
    "state": "init"
}
```

* **id** - unique file identificator. Leave field empty to generate unique file identificator
* **init_percent_complete** - percentage of initialization completed so far
* **file_size** - size of test file (in bytes)
* **path** - resource pathname
* **state** - initialization status of the block file
    * **none** - file initialization is in failed state
    * **init** - file initialization is in progress state
    * **ready** - file is ready for I/O load generation

## Block generator

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

* **id** - unique block generator identifier. Leave field empty to generate unique file identificator
* **resource_id** - unique device or file identifier chosen for I/O load generation
* **running** - indicates whether this generator is currently running
* **config** - configuration of block I/O load generation
    * **queue_depth** - maximum number of simultaneous (asynchronous) operations
    * **read_size** - number of bytes to use for each read operation
    * **reads_per_sec** - number of read operations to perform per second
    * **write_size** - number of bytes to use for each write operation
    * **writes_per_sec** - number of write operations to perform per second
    * **pattern** - I/O access pattern
        * **random** - access to the random point of the resource
        * **sequential** - sequential access to the resource from the beginning to the end
        * **reverse** - sequential reverse access to the resource from the end to the beginning

Leave *reads_per_sec* or *writes_per_sec* as zero value to ignore operation while I/O load generation

### Start block generator

Start block generator command returns Block Generator Result, created for the particular start. Response location header contains uri of created result.

## Block generator result

```json
{
    "active": true,
    "generator_id": "3f79937d-53ce-4411-435c-3139ea4a954c",
    "id": "24554373-e8df-4ad5-582c-21030e1d4cce",
    "read": {
        "bytes_actual": 0,
        "bytes_target": 0,
        "io_errors": 0,
        "latency": 0,
        "latency_max": 0,
        "latency_min": 0,
        "ops_actual": 0,
        "ops_target": 0
    },
    "timestamp": "1970-01-13T00:48:40.283877Z",
    "write": {
        "bytes_actual": 0,
        "bytes_target": 0,
        "io_errors": 0,
        "latency": 0,
        "latency_max": 0,
        "latency_min": -1,
        "ops_actual": 0,
        "ops_target": 0
    }
}
```

* **id** - unique block generator result identifier
* **generator_id** - block generator identifier that generated this result
* **active** - indicates whether the result is currently being updated
* **timestamp** - the ISO8601-formatted date of the last result update
* **read** - read block I/O operation statistics
* **write** - write block I/O operation statistics
    * **bytes_actual** - the actual number of bytes read or written
    * **bytes_target** - the intended number of bytes read or written
    * **io_errors** - the number of io_errors observed during reading or writing
    * **latency** - the total amount of time required to perform all operations (in nanoseconds)
    * **latency_max** - the maximum observed latency value (in nanoseconds)
    * **latency_min** - the minimum observed latency value (in nanoseconds)
    * **ops_actual** - the actual number of operations performed
    * **ops_target** - the intended number of operations performed
