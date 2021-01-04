#
# Makefile component for lwIP shared memory code
#

SOCKSRV_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(LW_REQ_VARS))

SOCKSRV_SRC_DIR := $(OP_ROOT)/src/modules/socket
SOCKSRV_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/socket_server
SOCKSRV_INC_DIR := $(OP_ROOT)/src/modules
SOCKSRV_LIB_DIR := $(OP_BUILD_ROOT)/lib

SOCKSRV_SOURCES :=
SOCKSRV_DEPENDS :=
SOCKSRV_LDLIBS  :=

include $(SOCKSRV_SRC_DIR)/directory.mk

SOCKSRV_OBJECTS := $(call op_generate_objects,$(SOCKSRV_SOURCES),$(SOCKSRV_OBJ_DIR))

SOCKSRV_LIBRARY := openperf_socket_server
SOCKSRV_TARGET := $(SOCKSRV_LIB_DIR)/lib$(SOCKSRV_LIBRARY).a

OP_INC_DIRS += $(SOCKSRV_INC_DIR)
OP_LDLIBS += -Wl,--whole-archive -l$(SOCKSRV_LIBRARY) -Wl,--no-whole-archive $(SOCKSRV_LDLIBS)

# Load external dependencies
-include $(SOCKSRV_OBJECTS:.o=.d)
$(call op_include_dependencies,$(SOCKSRV_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(SOCKSRV_SOURCES),SOCKSRV_SRC_DIR,SOCKSRV_OBJ_DIR,SOCKSRV_DEPENDS))
$(eval $(call op_generate_clean_rules,socket_server,SOCKSRV_TARGET,SOCKSRV_OBJECTS))

$(SOCKSRV_TARGET): $(SOCKSRV_OBJECTS)
	$(call op_link_library,$@,$(SOCKSRV_OBJECTS))

.PHONY: socket_server
socket_server: $(SOCKSRV_TARGET)
