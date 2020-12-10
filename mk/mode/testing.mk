#
# Makefile component to specify 'testing' build options
#

OP_COPTS += -Os

# The LLVM specific `line-tables-only` option is great for profiling.
# Unfortunately, gcc doesn't support it and DPDK needs gcc for cross compiling.
# Bummer.
ifeq ($(ARCH), $(HOST_ARCH)) # native compile
	OP_COPTS += -gline-tables-only
else
	OP_COPTS += -g
endif

OP_CPPFLAGS += -fstack-check
