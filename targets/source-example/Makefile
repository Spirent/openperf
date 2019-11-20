#
# Makefile to build an simple generic source example
#

OP_ROOT := $(realpath ../../)
OP_TARGET := source-example
include $(OP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(OP_BUILD_ROOT)/obj
TARGET_DIR := $(OP_BUILD_ROOT)/bin
TARGET_BIN := $(TARGET_DIR)/$(OP_TARGET)

all: $(OP_TARGET)

SOURCES := main.cpp
OBJECTS := $(call op_generate_objects,$(SOURCES),$(BUILD_DIR))

BUILD_DEPS := api config_file framework packetio socket_server
OP_PACKETIO_DRIVER ?= dpdk

# Pull in dependencies
-include $(OBJECTS:.o=.d)
$(call op_include_dependencies,$(BUILD_DEPS))

OP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
OP_CPPFLAGS += $(addprefix -I,$(sort $(OP_INC_DIRS)))
OP_LDFLAGS += $(addprefix -L,$(sort $(OP_LIB_DIRS)))
OP_LDOPTS += -static-libstdc++ -static-libgcc

# Build rules
$(eval $(call op_generate_build_rules,$(SOURCES),,BUILD_DIR,BUILD_DEPS))

$(BUILD_DIR)/main.o: OP_CPPFLAGS += -Wno-register

$(TARGET_BIN): $(OBJECTS) $(BUILD_DEPS)
	$(call op_link_binary,$@,$(OBJECTS))
	sudo -n /sbin/setcap CAP_IPC_LOCK,CAP_NET_ADMIN,CAP_NET_RAW=ep $@

.PHONY: $(OP_TARGET)
$(OP_TARGET): $(TARGET_BIN)

.PHONY: clean
clear:
	@rm -rf $(OP_BUILD_ROOT)
