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
# msse4.1/msse4.2: SSE4 instructions are required for some DPDK code
# mpclmul: PCLMUL isn't strictly necessary, but it drastically speeds up the signature CRC
# calculation, so we default it to "on" here. It's available on anything from the past decade.
#
# Check if the current optimizations give us those features.  If the don't, then we
# need to add them

OP_COMPILER_DEFINES := $(shell $(OP_CC) $(OP_COPTS) -dM -E - < /dev/null)

# Note: override is needed to change user defined variables
ifeq ($(filter $(OP_COMPILER_DEFINES),__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16),)
override OP_COPTS += -mcx16
endif

ifeq ($(filter $(OP_COMPILER_DEFINES),__SSE4_1__),)
override OP_COPTS += -msse4.1
endif

ifeq ($(filter $(OP_COMPILER_DEFINES),__SSE4_2__),)
override OP_COPTS += -msse4.2
endif

ifeq ($(filter $(OP_COMPILER_DEFINES),__PCLMUL__),)
override OP_COPTS += -mpclmul
endif


undefine OP_COMPILER_DEFINES

endif
