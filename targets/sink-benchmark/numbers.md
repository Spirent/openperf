# Receive Benchmark Numbers

## Test System

* CPU: AMD Ryzen Threadripper 2950X 16-Core Processor
* RAM: 32 GB, DDR4-2666, Unbuffered ECC, 4 x 8 GB DIMMs
* OS: Ubuntu 18.04.3 LTS (Linux kernel 4.15.0)
* NIC: Mellanox Technologies MT27800 Family [ConnectX-5]

Benchmark tests were run using the **sink-benchmark** target from the **openperf-prime**
repo at an unchecked-in version.

### CPU Architecture

The CPU used for these tests contains two processor dies connected via a
proprietary bus.  Each processor die comprises a NUMA domain and contains
both a memory controller and PCIe bus.  Additionally, each die contains a
pair of two, 4 core / 8 thread *chiplets*, which are labeled Core CompleX
(CCX) in the table below.

Cores that share hardware resources have indexes that differ by 16,
e.g. 0 and 16, 1 and 17, etc.

Processor | NUMA Node | Die | CCX | Core
----------|-----------|-----|-----|------
0|0|0|0|0
0|0|0|0|1
0|0|0|0|2
0|0|0|0|3
0|0|0|0|16
0|0|0|0|17
0|0|0|0|18
0|0|0|0|19
0|0|0|1|4
0|0|0|1|5
0|0|0|1|6
0|0|0|1|7
0|0|0|1|20
0|0|0|1|21
0|0|0|1|22
0|0|0|1|23
0|1|1|2|8
0|1|1|2|9
0|1|1|2|10
0|1|1|2|11
0|1|1|2|24
0|1|1|2|25
0|1|1|2|26
0|1|1|2|27
0|1|1|3|12
0|1|1|3|13
0|1|1|3|14
0|1|1|3|15
0|1|1|3|28
0|1|1|3|29
0|1|1|3|30
0|1|1|3|31

The test NIC is attached to NUMA domain 1.

## Basic Packet Reception

The receive path for this test was very simple:

1) Block on an interrupt waiting for packets
2) Dequeue all queued packets when the interrupt fires
3) Examine the buffer header for each packet to determine length.
4) Update packet/octet counters
5) Wait for more packets

All results are for 30 second transmission runs.  Per-second results
showed no significant deviation from the 30 second average.

The transmitter was configured to generate 512 streams so that streams
could be split across queues when using multi-queue tests.

CPU usage includes the system overhead for enabling/disabling interrupts.

### Frame Size Scaling

This test looks at maximum receive rate as the frame size is increased.
There appears to be a hard limit at just under 35 Mpps.

\# Threads | Core IDs | Frame Size | PPS | L2 Mbps | CPU %
-----------|----------|------------|-----|---------|------
1 | 8 |   64 | 34801351.30 | 23386.50 | 86.96
1 | 8 |  128 | 34188114.96 | 40478.73 | 87.02
1 | 8 |  256 | 32851417.51 | 72535.93 | 87.34
1 | 8 |  384 | 30940449.12 | 99997.01 | 87.06
1 | 8 |  512 | 23495962.36 | 99998.82 | 83.39
1 | 8 | 1024 | 11973160.36 | 99999.84 | 62.72
1 | 8 | 1280 |  9615321.70 | 99999.35 | 61.11
1 | 8 | 1518 |  8126990.56 | 99994.49 | 59.79

### Core Scaling

This test increases the number of cores and threads used to dequeue packets
from the NIC.  Each thread was responsible for a single NIC queue, so if 4 threads
were used for the test, then the NIC was configured with 4 receive queues.

#### 64 Byte Frames

\# Threads | Core IDs | Frame Size | PPS | L2 Mbps | CPU %
-----------|----------|------------|-----|---------|------
 1 |          8 | 64 |  34801351.30 | 23386.50 | 86.96
 2 |        8,9 | 64 |  62452177.13 | 41967.86 | 178.26
 2 |       8,24 | 64 |  62871097.34 | 42249.38 | 188.47
 3 |       8-10 | 64 |  76245615.81 | 51237.05 | 259.83
 4 |       8-11 | 64 |  87810988.34 | 59008.98 | 337.92
 4 |  8,9,24,25 | 64 |  73526953.12 | 49410.11 | 350.71
 6 | 8-10,24-26 | 64 |  78454613.50 | 52721.50 | 466.48
 8 | 8-11,24-27 | 64 |  90368890.43 | 60727.89 | 579.33
10 | 8-12,24-28 | 64 |  95340590.23 | 64068.88 | 632.14
12 | 8-13,24-29 | 64 | 101588247.21 | 68267.30 | 593.07

#### 128 Byte Frames

\# Threads | Core IDs | Frame Size | PPS | L2 Mbps | CPU %
-----------|----------|------------|-----|---------|------
 1 |          8 | 128 | 34188114.96 | 40478.73 | 87.02
 2 |        8-9 | 128 | 57365464.13 | 67920.71 | 175.33
 2 |       8,24 | 128 | 57857821.96 | 68503.66 | 186.10
 4 |       8-11 | 128 | 76883794.07 | 91030.41 | 331.92
 4 |  8,9,24,25 | 128 | 67629257.17 | 80073.04 | 346.91
 6 | 8-10,24-26 | 128 | 66120106.82 | 78286.21 | 459.28
 8 | 8-11,24-27 | 128 | 68866555.47 | 81538.00 | 547.19
10 | 8-12,24-28 | 128 | 70398405.01 | 83351.81 | 536.02
12 | 8-13,24-29 | 128 | 71711475.70 | 84906.39 | 506.13

#### 256 Byte Frames

\# Threads | Core IDs | Frame Size | PPS | L2 Mbps | CPU %
-----------|----------|------------|-----|---------|------
 1 |    8 | 256 | 32851417.51 | 72535.93 | 87.34
 2 |  8-9 | 256 | 40963437.44 | 90447.27 | 167.72
 2 | 8,24 | 256 | 40956654.59 | 90432.29 | 177.53

 Note: the 2 thread results represent the transmit limit of the
 test generator
