#
# x86_64 specific options
#

# At a minimum, we need the following x86 CPU flags to compile.
# mcx16: Enable compare-and-exchange for 16-byte aligned 128 bit objects
# mssse3: SSSE3 instructions are needed for some included DPDK headers, e.g. rte_memcpy.h
#
# Check if the current optimizations give us those features.  If the don't, then we
# need to add them

OP_COMPILER_DEFINES := $(shell $(OP_CC) $(OP_COPTS) -dM -E - < /dev/null)

# Note: override is needed to change user defined variables
ifeq ($(filter $(OP_COMPILER_DEFINES),__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16),)
override OP_COPTS += -mcx16
endif

ifeq ($(filter $(OP_COMPILER_DEFINES),__SSSE3__),)
override OP_COPTS += -mssse3
endif

undefine OP_COMPILER_DEFINES
