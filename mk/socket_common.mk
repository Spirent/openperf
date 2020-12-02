#
# Makefile component for lwIP shared memory code
#

SOCK_COMMON_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(LW_REQ_VARS))

SOCK_COMMON_SRC_DIR := $(OP_ROOT)/src/modules/socket
SOCK_COMMON_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/socket_common
SOCK_COMMON_INC_DIR := $(OP_ROOT)/src/modules
SOCK_COMMON_LIB_DIR := $(OP_BUILD_ROOT)/lib

SOCK_COMMON_SOURCES :=
SOCK_COMMON_DEPENDS :=
SOCK_COMMON_LDLIBS  :=

include $(SOCK_COMMON_SRC_DIR)/directory.mk

SOCK_COMMON_OBJECTS := $(call op_generate_objects,$(SOCK_COMMON_SOURCES),$(SOCK_COMMON_OBJ_DIR))

SOCK_COMMON_LIBRARY := openperf_socket_common
SOCK_COMMON_TARGET := $(SOCK_COMMON_LIB_DIR)/lib$(SOCK_COMMON_LIBRARY).a

OP_INC_DIRS += $(SOCK_COMMON_INC_DIR) $(OP_ROOT)/src/framework
OP_LDLIBS += -Wl,--whole-archive -l$(SOCK_COMMON_LIBRARY) -Wl,--no-whole-archive $(SOCK_COMMON_LDLIBS)

# Load external dependencies, too
-include $(SOCK_COMMON_OBJECTS:.o=.d)
$(call op_include_dependencies,$(SOCK_COMMON_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(SOCK_COMMON_SOURCES),SOCK_COMMON_SRC_DIR,SOCK_COMMON_OBJ_DIR,SOCK_COMMON_DEPENDS,SOCK_COMMON_FLAGS))
$(eval $(call op_generate_clean_rules,socket_common,SOCK_COMMON_TARGET,SOCK_COMMON_OBJECTS))

$(SOCK_COMMON_TARGET): $(SOCK_COMMON_OBJECTS)
	$(call op_link_library,$@,$(SOCK_COMMON_OBJECTS))

.PHONY: socket_common
socket_common: $(SOCK_COMMON_TARGET)
