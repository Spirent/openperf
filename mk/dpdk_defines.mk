#
# Makefile component for proper DPDK 'machine' defines
#
# Note: this only generates a subset of DPDK's cpu related defines as we only
# need the ones that influence the behavior of rte_memcpy.
#

DPDK_COMPILER_DEFINES := $(shell $(OP_CC) $(OP_COPTS) $(OP_CROSSFLAGS) -dM -E - < /dev/null)

###
# x86 flags
###
DPDK_MACHINE_FLAGS :=
DPDK_CONFIG_FLAGS :=

ifneq ($(filter $(DPDK_COMPILER_DEFINES),__AVX2__),)
DPDK_MACHINE_FLAGS += AVX2
endif

ifneq ($(filter $(DPDK_COMPILER_DEFINES),__AVX512F__),)
DPDK_MACHINE_FLAGS += AVX512F
endif

###
# arm flags
###
ifneq ($(filter $(DPDK_COMPILER_DEFINES),__aarch64__),)
DPDK_CONFIG_FLAGS += FORCE_INTRINSICS ARCH_ARM64
endif

ifneq ($(filter $(DPDK_COMPILER_DEFINES),__ARM_NEON),)
DPDK_MACHINE_FLAGS += NEON
endif

ifneq ($(filter $(DPDK_COMPILER_DEFINES), __ARM_FEATURE_CRC32),)
DPDK_MACHINE_FLAGS += CRC32
endif

ifneq ($(filter $(DPDK_COMPILER_DEFINES), __ARM_FEATURE_CRYPTO),)
DPDK_MACHINE_FLAGS += AES
DPDK_MACHINE_FLAGS += PMULL
DPDK_MACHINE_FLAGS += SHA1
DPDK_MACHINE_FLAGS += SHA2
endif

undefine DPDK_COMPILER_DEFINES

DPDK_DEFINES += \
	$(addprefix -DRTE_MACHINE_CPUFLAG_,$(DPDK_MACHINE_FLAGS)) \
	$(addprefix -DRTE_, $(DPDK_CONFIG_FLAGS))
