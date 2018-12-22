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
SOCKCLI_OBJECTS :=
SOCKCLI_DEPENDS :=
SOCKCLI_LDLIBS  :=

include $(SOCKCLI_SRC_DIR)/module.mk

SOCKCLI_OBJECTS += $(patsubst %, $(SOCKCLI_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(SOCKCLI_SOURCES))))

SOCKCLI_OBJECTS += $(patsubst %, $(SOCKCLI_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(SOCKCLI_SOURCES))))

SOCKCLI_CPPFLAGS := -I$(ICP_ROOT)/deps/lwip/src/include

SOCKCLI_LIBRARY := icpsocket_client
SOCKCLI_TARGET := $(SOCKCLI_LIB_DIR)/lib$(SOCKCLI_LIBRARY).a

-include $(SOCKCLI_OBJECTS:.o=.d)

ICP_INC_DIRS += $(SOCKCLI_INC_DIR)
ICP_LIB_DIRS += $(SOCKCLI_LIB_DIR)
ICP_LDLIBS += -l$(SOCKCLI_LIBRARY) $(SOCKCLI_LDLIBS)

# Load external dependencies, too
$(foreach DEP, $(SOCKCLI_DEPENDS), $(eval include $(ICP_ROOT)/mk/$(DEP).mk))

###
# Build rules
###
$(SOCKCLI_OBJECTS): | $(SOCKCLI_DEPENDS)

$(SOCKCLI_OBJ_DIR)/%.o: $(SOCKCLI_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(SOCKCLI_OBJ_DIR)/%.o: $(SOCKCLI_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(SOCKCLI_TARGET): $(SOCKCLI_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(SOCKCLI_OBJECTS))

.PHONY: socket_client
socket_client: $(SOCKCLI_TARGET)

.PHONY: clean_socket_client
clean_socket_client:
	@rm -rf $(SOCKCLI_OBJ_DIR) $(SOCKCLI_TARGET)
clean: clean_socket_client
