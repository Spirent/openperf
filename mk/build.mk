#
# Base Makefile component for all builds
#

$(call icp_check_vars,ICP_ROOT ICP_TARGET)

# Generate platform/arch/mode tuple and load corresponding sub-makefiles
PLATFORM ?= $(shell uname -s | tr [A-Z] [a-z])
ifeq (,$(wildcard $(ICP_ROOT)/mk/platform/$(PLATFORM).mk))
$(error unsupported build platform $(PLATFORM))
endif
include $(ICP_ROOT)/mk/platform/$(PLATFORM).mk

ARCH ?= $(shell uname -m)
ifeq (,$(wildcard $(ICP_ROOT)/mk/arch/$(ARCH).mk))
$(error unsupported build architecture $(ARCH))
endif
include $(ICP_ROOT)/mk/arch/$(ARCH).mk

MODE ?= testing
ifeq (,$(wildcard $(ICP_ROOT)/mk/mode/$(MODE).mk))
$(error unsupported build mode $(MODE))
endif
include $(ICP_ROOT)/mk/mode/$(MODE).mk

ICP_BUILD_ROOT := $(ICP_ROOT)/build/$(ICP_TARGET)-$(PLATFORM)-$(ARCH)-$(MODE)
