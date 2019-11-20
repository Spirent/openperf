#
# Makefile component for lwIP shared memory code
#

SOCKTEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(LW_REQ_VARS))

SOCKTEST_SRC_DIR := $(OP_ROOT)/src/modules/socket
SOCKTEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/socket_server
SOCKTEST_INC_DIR := $(OP_ROOT)/src/modules
SOCKTEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

SOCKTEST_SOURCES :=
SOCKTEST_DEPENDS :=
SOCKTEST_LDLIBS  :=

include $(SOCKTEST_SRC_DIR)/directory.mk

SOCKTEST_OBJECTS := $(call op_generate_objects,$(SOCKTEST_SOURCES),$(SOCKTEST_OBJ_DIR))

SOCKTEST_LIBRARY := openperf_socket_test
SOCKTEST_TARGET := $(SOCKTEST_LIB_DIR)/lib$(SOCKTEST_LIBRARY).a

OP_INC_DIRS += $(SOCKTEST_INC_DIR)
OP_LIB_DIRS += $(SOCKTEST_LIB_DIR)
OP_LDLIBS += -l$(SOCKTEST_LIBRARY) $(SOCKTEST_LDLIBS)

# Load external dependencies
-include $(SOCKTEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(SOCKTEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(SOCKTEST_SOURCES),SOCKTEST_SRC_DIR,SOCKTEST_OBJ_DIR,SOCKTEST_DEPENDS))
$(eval $(call op_generate_clean_rules,socket_test,SOCKTEST_TARGET,SOCKTEST_OBJECTS))

$(SOCKTEST_TARGET): $(SOCKTEST_OBJECTS)
	$(call op_link_library,$@,$(SOCKTEST_OBJECTS))

.PHONY: socket_test
socket_test: $(SOCKTEST_TARGET)
