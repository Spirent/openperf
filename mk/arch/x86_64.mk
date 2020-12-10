#
# x86_64 specific options
#

ifneq ($(ARCH), $(HOST_ARCH)) # cross compile

ifeq ($(HOST_ARCH), $(shell uname -m))
	HOST_TARGET := x86_64-$(PLATFORM)-gnu
else
	$(error unsupported cross compile: $(HOST_ARCH) --> $(ARCH))
endif

else # native compile

OP_MACHINE_ARCH ?= native
OP_COPTS += -march=$(OP_MACHINE_ARCH)  # applied to both c and c++ targets

# We specifically use a 128 bit wide AVX1 target, even though AVX supports
# 256 bit wide registers, for improved performance on older AMD chips.  Those chips
# emulate AVX2 instructions using 2 x 128 bit wide registers (and hence 256 bit wide
# AVX runs more slowly than 128 bit wide AVX in some cases).
OP_ISPC_TARGETS := \
	sse4-i32x4 \
	avx1-i32x4 \
	avx2-i32x8 \
	avx512skx-i32x16

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

endif
