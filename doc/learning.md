# OpenPerf Learning (ARP/ND) Support

## Background

OpenPerf uses ARP/ND learning to resolve destination MAC addresses for traffic generators bound to interfaces. Traffic generators bound to ports do not support automated destination MAC resolution.

## Theory of Operation

OpenPerf performs learning process automatically when a client `POST`s a generator resource that's attached to an interface. The packet generator module extracts unique layer 3 destination addresses and passes them to the learning module. The learning module uses ARP/ND capabilities of `lwip` stack to perform the required packet exchanges. Once all addresses have been resolved and/or timeouts exceeded, learning module returns resolved information to the packet generator module. The packet generator module then continues the existing process of creating the generator resource.

## REST Client View

Learning module adds two endpoints: `/learning` and `/learning-results`.

The `/learning` endpoint represents client requests to perform learning on a specified list of generators. *[Open to suggestions on what to call each resource. Learning transaction? request? package?]* A `POST` request initiates learning. A `GET` request gives status information. A `DELETE` request cancels an in-progress learning request or removes a completed one.

The `/learning-results` endpoint represents results from a learning request. A `GET` request gives resolved addresses and metadata information. A `DELETE` request removes a completed learning result.

REST API clients will notice some differences when creating packet generators bound to interfaces. For instance if the packet generator module intends to perform learning it will return HTTP code `202 Accepted` instead of `201 Created`. The `202` response includes IDs of the generator under construction and the learning request from `/learning`. Clients must wait for generator resource to signal learning done before starting traffic.

## Implementation Details

### Learning Module

OpenPerf has learning as a separate mode to reduce dependencies between modules. Another module (usually packet generator) passes a list of destination IP addresses and an interface ID to learning module. Learning module uses ARP/ND functionality of `lwip` to resolve MAC addresses. Learning module will not "manually" create and exchange ARP/ND packets. Once all addresses have resolved or timed out learning module returns results to the calling module.

### `lwip` requests

ARP/ND requests to `lwip` are asynchronous in nature and require polling to determine if they've completed. Learning module uses state machines and an event loop to track each request. State machines have three basic states: { `resolving`, `completed`, `timed_out` }.

### Packet Generator Updates

Once learning is done the packet generator module updates the destination MAC for transmit flows. This update is done to the expanded packet headers. *[and original config?]*

## Project Planning (For internal use only)

(Sections from here down are for internal project purposes and will not be present in the final version.)

### Tasks/Schedule

| Task                                           | Description                                                     | Duration | Dates |
| ---------------------------------------------- | --------------------------------------------------------------- | -------- | ----- |
| Add support for generators bound to interfaces | Right now OpenPerf only supports generators bound to ports      | Xw       | tbd   |
| REST API                                       | Changes to swagger spec for `/learning` and `/learning-results` | Xw       | tbd   |
| Learning Module skeleton                       | Add new module, create REST translators, add to build system    | Xw       | tbd   |
| Connect `lwip` stack and learning module       | Define interface between the two. Maybe changes on `lwip` side  | Xw       | tbd   |
| Learning requests                              | Request state machine, request polling                          | Xw       | tbd   |
| Packet generator setup changes                 | Return `202`, skip some configuration while learning            | Xw       | tbd   |
| Packet generator modify frame content          | Apply resolved MACs to packet headers                           | Xw       | tbd   |

### Notes

1. Project will be iterative. No way everything will be implemented by 12/31/2020. But we can work with customer to have incremental drops.

2. For the first release `/learning` will only support `GET`. `POST` requests to be added later. Users can only do learning when creating a generator; they cannot manually trigger learning.

3.

### Open Questions

1. What does `GET /generators` return? Original configuration as `POST`ed by the user or original + resolved MAC addresses?

2. How do we handle `POST` of a completely new generator vs `POST` to update an existing one? Leverage `lwip` ARP/ND cache?

3. When `POST /generators` returns `202: Accepted` does the generator get created and marked as "learning in progress"? Or does it not get created at all until learning completes?

4. Error handling when ARP/ND fails. Fall back on default MAC? Mark generator as invalid?

5. Keep completed `/learning` requests? Or just keep the associated results?

6. MAC address timeout? As in do resolved MACs time out like on real systems?
