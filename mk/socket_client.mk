#
# Makefile component for lwIP shared memory code
#

SOCKCLI_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(LW_REQ_VARS))

SOCKCLI_SRC_DIR := $(ICP_ROOT)/src/modules/socket
SOCKCLI_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/modules/socket_server
SOCKCLI_INC_DIR := $(ICP_ROOT)/src/modules
SOCKCLI_LIB_DIR := $(ICP_BUILD_ROOT)/lib

SOCKCLI_SOURCES :=
SOCKCLI_DEPENDS :=
SOCKCLI_LDLIBS  :=

include $(SOCKCLI_SRC_DIR)/directory.mk

SOCKCLI_OBJECTS := $(call icp_generate_objects,$(SOCKCLI_SOURCES),$(SOCKCLI_OBJ_DIR))

SOCKCLI_LIBRARY := icp_socket_client
SOCKCLI_TARGET := $(SOCKCLI_LIB_DIR)/lib$(SOCKCLI_LIBRARY).a

# The client code depends on core/icp_uuid.h.  This is a header only object,
# so it's safe to include without pulling in the whole framework.  However,
# any clients we have will need to find that path has well, so we need to
# add the framework path to our exported header list also.
SOCKCLI_FLAGS := -I$(ICP_ROOT)/src/framework

ICP_INC_DIRS += $(SOCKCLI_INC_DIR) $(ICP_ROOT)/src/framework
ICP_LIB_DIRS += $(SOCKCLI_LIB_DIR)
ICP_LDLIBS += -l$(SOCKCLI_LIBRARY) $(SOCKCLI_LDLIBS)

# Load external dependencies, too
-include $(SOCKCLI_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(SOCKCLI_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(SOCKCLI_SOURCES),SOCKCLI_SRC_DIR,SOCKCLI_OBJ_DIR,SOCKCLI_DEPENDS,SOCKCLI_FLAGS))
$(eval $(call icp_generate_clean_rules,socket_client,SOCKCLI_TARGET,SOCKCLI_OBJECTS))

$(SOCKCLI_TARGET): $(SOCKCLI_OBJECTS)
	$(call icp_link_library,$@,$(SOCKCLI_OBJECTS))

.PHONY: socket_client
socket_client: $(SOCKCLI_TARGET)
