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
SOCKTEST_OBJECTS :=
SOCKTEST_DEPENDS :=
SOCKTEST_LDLIBS  :=

include $(SOCKTEST_SRC_DIR)/module.mk

SOCKTEST_OBJECTS += $(patsubst %, $(SOCKTEST_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(SOCKTEST_SOURCES))))

SOCKTEST_OBJECTS += $(patsubst %, $(SOCKTEST_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(SOCKTEST_SOURCES))))

SOCKTEST_LIBRARY := icpsocket_test
SOCKTEST_TARGET := $(SOCKTEST_LIB_DIR)/lib$(SOCKTEST_LIBRARY).a

-include $(SOCKTEST_OBJECTS:.o=.d)

ICP_INC_DIRS += $(SOCKTEST_INC_DIR)
ICP_LIB_DIRS += $(SOCKTEST_LIB_DIR)
ICP_LDLIBS += -l$(SOCKTEST_LIBRARY) $(SOCKTEST_LDLIBS)

# Load external dependencies, too
$(foreach DEP, $(SOCKTEST_DEPENDS), $(eval include $(ICP_ROOT)/mk/$(DEP).mk))

###
# Build rules
###
$(SOCKTEST_OBJECTS): | $(SOCKTEST_DEPENDS)

$(SOCKTEST_OBJ_DIR)/%.o: $(SOCKTEST_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(SOCKTEST_OBJ_DIR)/%.o: $(SOCKTEST_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(SOCKTEST_TARGET): $(SOCKTEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(SOCKTEST_OBJECTS))

.PHONY: socket_test
socket_test: $(SOCKTEST_TARGET)

.PHONY: clean_socket_test
clean_socket_test:
	@rm -rf $(SOCKTEST_OBJ_DIR) $(SOCKTEST_TARGET)
clean: clean_socket_test
