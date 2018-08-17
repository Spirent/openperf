#
# x86_64 specific options
#

# Enable compare-and-exchange for 16-byte aligned 128 bit objects
X86_OPTS += -mcx16

# Sandybridge equivalent is our minimum target
X86_OPTS += -march=sandybridge

ICP_CFLAGS += ${X86_OPTS}
ICP_CXXFLAGS += ${X86_OPTS}
