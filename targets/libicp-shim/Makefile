#
# Makefile to build inception shim library
#

ICP_ROOT := $(realpath ../../)
ICP_TARGET := libicp-shim
include $(ICP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(ICP_BUILD_ROOT)/obj
TARGET_DIR := $(ICP_BUILD_ROOT)/lib
TARGET_LIB := $(TARGET_DIR)/$(ICP_TARGET).so

SOURCES := \
	icp-shim.cpp \
	libc_wrapper.cpp

OBJECTS := $(call icp_generate_objects,$(SOURCES),$(BUILD_DIR))
BUILD_DEPS := socket_client

all: $(TARGET_LIB)

# Pull in dependencies
-include $(OBJECTS:.o=.d)
$(call icp_include_dependencies,$(BUILD_DEPS))

ICP_CPPFLAGS += -fPIC $(addprefix -I,$(sort $(ICP_INC_DIRS)))
ICP_LDFLAGS += $(addprefix -L,$(sort $(ICP_LIB_DIRS)))
ICP_LDOPTS += -static-libstdc++ -static-libgcc -shared
ICP_LDLIBS += -ldl

# Build rules
$(eval $(call icp_generate_build_rules,$(SOURCES),,BUILD_DIR,BUILD_DEPS))

$(TARGET_LIB): $(OBJECTS) $(BUILD_DEPS)
	$(call icp_link_library,$@,$(OBJECTS))

.PHONY: $(ICP_TARGET)
$(ICP_TARGET): $(TARGET_LIB)

.PHONY: clean
clean:
	@rm -rf $(ICP_BUILD_ROOT)
