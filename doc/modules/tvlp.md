# Time Varying Load Profile

Time Varying Load Profile (TVLP) provides a mechanism to program an OpenPerf instance to perform Block, Memory, CPU and Packet modules configuration adjustments at specific times.

## Configuration

Configuration primarily consists of a lists of time offsets and values for configuraton options for Block, Memory, CPU and Packet generators.

Additionally, clients may specify a specific UTC ISO8601-formatted time to start replaying the configured profile.

### States

The TVLP module contains 4 distinct states:

- READY -- TVLP contains a valid configuration and is ready to be started
- COUNTDOWN -- TVLP has been given a start time in the future and is waiting to start replaying a profile
- RUNNING -- TVLP module is replaying a profile
- ERROR -- TVLP module encountered a runtime error

#### State Diagram

State transitions behave as follows:

![TVLP State Diagram](../images/tvlp-state-diagram.png)

After a profile has completed its run, the `state` revert back to READY.

### Keys

The TVLP module responds to the following configuration keys:

- **id** -- unique TVLP identifier. Leave field empty to generate value.
- **time.start** -- ISO8601-formatted TVLP profile start time. Only available when `state` = COUNTDOWN or RUNNING (read only)
- **time.length** -- length of current tvlp profile in ns (read only)
- **time.offset** -- current offset in ns. Only valid when `state` = RUNNING (read only)
- **state** -- current TVLP profile `state`, one of: READY, COUNTDOWN, RUNNING, or ERROR (read only)
- **error** -- read only string describing error condition; only when `state` == ERROR (read only)
- **profile** -- TVLP profile data

## TVLP Configuration Diagram

Each generator module configured by TVLP profile as follows:

![TVLP Configuration Diagram](../images/tvlp-scheme.png)

## TVLP Result

TVLP creates new _Result_ with new unique ID on each Start request. While TVLP configuration is running, the result is being updated after each generation iteration. After the Profile has completed the result receives final statistics values.

## Commands flow

### Create resources for each Profile entry

### Create TVLP Configuration

[Block Generator Configuration](block.md)

[Memory Generator Configuration](memory.md)

[CPU Generator Configuration](cpu.md)

[Packet Generator Configuration](../dev-guide/module-packetio.md)

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/tvlp' \
--header 'Content-Type: application/json' \
--data-raw ' {
        "id": "qwe2",
        "profile": {
            "block": {
                "series": [
                    {
                        "config": {$BLOCK_GENERATOR_CONFIGURATION},
                        "length": 1000000000,
                        "resource_id": "$BLOCK_RESOURCE_ID"
                    }
                ]
            },
            "memory": {
                "series": [
                    {
                        "config": {$MEMORY_GENERATOR_CONFIGURATION},
                        "length": 1000000000
                    }
                ]
            },
            "cpu": {
                "series": [
                    {
                        "config": {$CPU_GENERATOR_CONFIGURATION},
                        "length": 1000000000
                    }
                ]
            },
            "packet": {
                "series": [
                    {
                        "config": {$PACKET_GENERATOR_CONFIGURATION},
                        "length": 100000000,
                        "target_id": "port0"
                    }
                ]
            }
        },
        "state": "ready",
        "time": {
            "length": 100000000
        }
    }'
```

### Start existing TVLP Configuration

```bash
curl --location --request POST '<OPENPERF_HOST>:<OPENPERF_PORT>/tvlp/:id/start?time=2020-10-01T00:00:00.000000Z'
```

### Check the created TVLP Result

```bash
curl --location --request GET '<OPENPERF_HOST>:<OPENPERF_PORT>/tvlp-results/:id'
```
