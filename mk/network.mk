#
# Makefile component for network generator
#

NETWORK_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(NETWORK_REQ_VARS))

NETWORK_SRC_DIR := $(OP_ROOT)/src/modules/network
NETWORK_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/network
NETWORK_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(NETWORK_LIB_DIR)

NETWORK_SOURCES :=
NETWORK_DEPENDS :=
NETWORK_LDLIBS :=

include $(NETWORK_SRC_DIR)/directory.mk

NETWORK_OBJECTS := $(call op_generate_objects,$(NETWORK_SOURCES),$(NETWORK_OBJ_DIR))

NETWORK_LIBRARY := openperf_network
NETWORK_TARGET := $(NETWORK_LIB_DIR)/lib$(NETWORK_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(NETWORK_LIBRARY) -Wl,--no-whole-archive $(NETWORK_LDLIBS)

# Load external dependencies
-include $(NETWORK_OBJECTS:.o=.d)
$(call op_include_dependencies,$(NETWORK_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(NETWORK_SOURCES),NETWORK_SRC_DIR,NETWORK_OBJ_DIR,NETWORK_DEPENDS))
$(eval $(call op_generate_clean_rules,network,NETWORK_TARGET,NETWORK_OBJECTS))

$(NETWORK_TARGET): $(NETWORK_OBJECTS)
	$(call op_link_library,$@,$(NETWORK_OBJECTS))

.PHONY: network
network: $(NETWORK_TARGET)
