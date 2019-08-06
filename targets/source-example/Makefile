#
# Makefile to build an simple generic source example
#

ICP_ROOT := $(realpath ../../)
ICP_TARGET := source-example
include $(ICP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(ICP_BUILD_ROOT)/obj
TARGET_DIR := $(ICP_BUILD_ROOT)/bin
TARGET_BIN := $(TARGET_DIR)/$(ICP_TARGET)

all: $(ICP_TARGET)

SOURCES := main.cpp
OBJECTS := $(call icp_generate_objects,$(SOURCES),$(BUILD_DIR))

BUILD_DEPS := api config_file framework packetio socket_server
ICP_PACKETIO_DRIVER ?= dpdk

# Pull in dependencies
-include $(OBJECTS:.o=.d)
$(call icp_include_dependencies,$(BUILD_DEPS))

ICP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
ICP_CPPFLAGS += $(addprefix -I,$(sort $(ICP_INC_DIRS)))
ICP_LDFLAGS += $(addprefix -L,$(sort $(ICP_LIB_DIRS)))
ICP_LDOPTS += -static-libstdc++ -static-libgcc

# Build rules
$(eval $(call icp_generate_build_rules,$(SOURCES),,BUILD_DIR,BUILD_DEPS))

$(BUILD_DIR)/main.o: ICP_CPPFLAGS += -Wno-register

$(TARGET_BIN): $(OBJECTS) $(BUILD_DEPS)
	$(call icp_link_binary,$@,$(OBJECTS))
	-sudo -n /sbin/setcap CAP_IPC_LOCK,CAP_NET_RAW,CAP_SYS_ADMIN,CAP_DAC_OVERRIDE,CAP_SYS_PTRACE=epi $@

.PHONY: $(ICP_TARGET)
$(ICP_TARGET): $(TARGET_BIN)

.PHONY: clean
clear:
	@rm -rf $(ICP_BUILD_ROOT)
