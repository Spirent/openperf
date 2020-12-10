#
# AArch64 specific build options
#

ifeq ($(ARCH), aarch64)
		OP_ISPC_TARGETS := neon-i32x4

ifneq ($(ARCH), $(HOST_ARCH)) # cross compile
	  TARGET := aarch64-$(PLATFORM)-gnu
		OP_CROSSFLAGS += --target=$(TARGET) --sysroot=$(OP_SYSROOT)
		OP_CONFIG_OPTS += --host=$(TARGET) --build=$(HOST_TARGET)
		OP_COPTS += -march=armv8.2-a
endif

endif
