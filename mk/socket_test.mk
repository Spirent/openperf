#
# Makefile component for lwIP shared memory code
#

SOCKTEST_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(LW_REQ_VARS))

SOCKTEST_SRC_DIR := $(ICP_ROOT)/src/modules/socket
SOCKTEST_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/modules/socket_server
SOCKTEST_INC_DIR := $(ICP_ROOT)/src/modules
SOCKTEST_LIB_DIR := $(ICP_BUILD_ROOT)/lib

SOCKTEST_SOURCES :=
SOCKTEST_DEPENDS :=
SOCKTEST_LDLIBS  :=

include $(SOCKTEST_SRC_DIR)/directory.mk

SOCKTEST_OBJECTS := $(call icp_generate_objects,$(SOCKTEST_SOURCES),$(SOCKTEST_OBJ_DIR))

SOCKTEST_LIBRARY := icpsocket_test
SOCKTEST_TARGET := $(SOCKTEST_LIB_DIR)/lib$(SOCKTEST_LIBRARY).a

ICP_INC_DIRS += $(SOCKTEST_INC_DIR)
ICP_LIB_DIRS += $(SOCKTEST_LIB_DIR)
ICP_LDLIBS += -l$(SOCKTEST_LIBRARY) $(SOCKTEST_LDLIBS)

# Load external dependencies
-include $(SOCKTEST_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(SOCKTEST_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(SOCKTEST_SOURCES),SOCKTEST_SRC_DIR,SOCKTEST_OBJ_DIR,SOCKTEST_DEPENDS))
$(eval $(call icp_generate_clean_rules,socket_test,SOCKTEST_TARGET,SOCKTEST_OBJECTS))

$(SOCKTEST_TARGET): $(SOCKTEST_OBJECTS)
	$(call icp_link_library,$@,$(SOCKTEST_OBJECTS))

.PHONY: socket_test
socket_test: $(SOCKTEST_TARGET)
