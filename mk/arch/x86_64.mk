#
# x86_64 specific options
#

# Enable compare-and-exchange for 16-byte aligned 128 bit objects
X86_OPTS += -mcx16

# SSE4.2 provided the CRC32 instruction, which DPDK needs
X86_OPTS += -msse4.2

ICP_COPTS += ${X86_OPTS}
