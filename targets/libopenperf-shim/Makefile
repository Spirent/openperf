#
# Makefile to build the openperf shim library
#

OP_ROOT := $(realpath ../../)
OP_TARGET := libopenperf-shim
include $(OP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(OP_BUILD_ROOT)/obj
TARGET_DIR := $(OP_BUILD_ROOT)/lib
TARGET_LIB := $(TARGET_DIR)/$(OP_TARGET).so

SOURCES := \
	openperf-shim.cpp \
	libc_wrapper.cpp

OBJECTS := $(call op_generate_objects,$(SOURCES),$(BUILD_DIR))
BUILD_DEPS := socket_client

all: $(TARGET_LIB)

# Pull in dependencies
-include $(OBJECTS:.o=.d)
$(call op_include_dependencies,$(BUILD_DEPS))

OP_CPPFLAGS += -fPIC $(addprefix -I,$(sort $(OP_INC_DIRS)))
OP_LDFLAGS += $(addprefix -L,$(sort $(OP_LIB_DIRS)))
OP_LDOPTS += -static-libstdc++ -static-libgcc -shared
OP_LDLIBS += -ldl

# Build rules
$(eval $(call op_generate_build_rules,$(SOURCES),,BUILD_DIR,BUILD_DEPS))

$(TARGET_LIB): $(OBJECTS) $(BUILD_DEPS)
	$(call op_link_library,$@,$(OBJECTS))

.PHONY: $(OP_TARGET)
$(OP_TARGET): $(TARGET_LIB)

.PHONY: clean
clean:
	@rm -rf $(OP_BUILD_ROOT)
