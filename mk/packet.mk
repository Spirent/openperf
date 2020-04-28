#
# Makefile component for the packet library
#

PKT_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PKT_REQ_VARS))

PKT_SRC_DIR := $(OP_ROOT)/src/lib/packet
PKT_INC_DIR := $(dir $(PKT_SRC_DIR))
PKT_OBJ_DIR := $(OP_BUILD_ROOT)/obj/lib/packet
PKT_LIB_DIR := $(OP_BUILD_ROOT)/lib

PKT_SOURCES :=

include $(PKT_SRC_DIR)/directory.mk

PKT_OBJECTS := $(call op_generate_objects,$(PKT_SOURCES),$(PKT_OBJ_DIR))

PKT_LIBRARY := openperf_packet
PKT_TARGET := $(PKT_LIB_DIR)/lib$(PKT_LIBRARY).a

# Load external dependencies
-include $(PKT_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PKT_DEPENDS))

OP_INC_DIRS += $(PKT_INC_DIR)
OP_LIB_DIRS += $(PKT_LIB_DIR)
OP_LDLIBS += -l$(PKT_LIBRARY)

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PKT_SOURCES),PKT_SRC_DIR,PKT_OBJ_DIR,PKT_DEPENDS))
$(eval $(call op_generate_clean_rules,packet,PKT_TARGET,PKT_OBJECTS))

$(PKT_TARGET): $(PKT_OBJECTS)
	$(call op_link_library,$@,$(PKT_OBJECTS))

.PHONY: packet
packet: $(PKT_TARGET)
