#
# Base Makefile component for all builds
#

$(call op_check_vars,OP_ROOT OP_TARGET)

# Generate platform/arch/mode tuple and load corresponding sub-makefiles
PLATFORM ?= $(shell uname -s | tr [A-Z] [a-z])
ifeq (,$(wildcard $(OP_ROOT)/mk/platform/$(PLATFORM).mk))
$(error unsupported build platform $(PLATFORM))
endif
include $(OP_ROOT)/mk/platform/$(PLATFORM).mk

HOST_ARCH ?= $(shell uname -m)
ARCH ?= $(HOST_ARCH)

ifneq ($(HOST_ARCH), $(ARCH))
	ifeq (,$(wildcard $(OP_ROOT)/mk/arch/$(HOST_ARCH).mk))
		$(error unsupported host architecture $(HOST_ARCH))
	endif
	include $(OP_ROOT)/mk/arch/$(HOST_ARCH).mk
endif

ifeq (,$(wildcard $(OP_ROOT)/mk/arch/$(ARCH).mk))
$(error unsupported build architecture $(ARCH))
endif
include $(OP_ROOT)/mk/arch/$(ARCH).mk

MODE ?= testing
ifeq (,$(wildcard $(OP_ROOT)/mk/mode/$(MODE).mk))
$(error unsupported build mode $(MODE))
endif
include $(OP_ROOT)/mk/mode/$(MODE).mk

OP_BUILD_ROOT := $(OP_ROOT)/build/$(OP_TARGET)-$(PLATFORM)-$(ARCH)-$(MODE)
