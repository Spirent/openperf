#
# x86_64 specific options
#

# At a minimum, we need the following x86 CPU flags to compile.
# mcx16: Enable compare-and-exchange for 16-byte aligned 128 bit objects
# mssse3: SSSE3 instructions are needed for some included DPDK headers, e.g. rte_memcpy.h
#
# Check if the current optimizations give us those features.  If the don't, then we
# need to add them

ICP_COMPILER_DEFINES := $(shell $(ICP_CC) $(ICP_COPTS) -dM -E - < /dev/null)

# Note: override is needed to change user defined variables
ifeq ($(filter $(ICP_COMPILER_DEFINES),__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16),)
override ICP_COPTS += -mcx16
endif

ifeq ($(filter $(ICP_COMPILER_DEFINES),__SSSE3__),)
override ICP_COPTS += -mssse3
endif

undefine ICP_COMPILER_DEFINES
