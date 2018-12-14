#
# Makefile component for lwIP shared memory code
#

SOCKSRV_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(LW_REQ_VARS))

SOCKSRV_SRC_DIR := $(ICP_ROOT)/src/modules/socket
SOCKSRV_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/modules/socket_server
SOCKSRV_INC_DIR := $(ICP_ROOT)/src/modules
SOCKSRV_LIB_DIR := $(ICP_BUILD_ROOT)/lib

SOCKSRV_SOURCES :=
SOCKSRV_OBJECTS :=
SOCKSRV_DEPENDS :=
SOCKSRV_LDLIBS  :=

include $(SOCKSRV_SRC_DIR)/module.mk

SOCKSRV_OBJECTS += $(patsubst %, $(SOCKSRV_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(SOCKSRV_SOURCES))))

SOCKSRV_OBJECTS += $(patsubst %, $(SOCKSRV_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(SOCKSRV_SOURCES))))

SOCKSRV_CPPFLAGS := -I$(ICP_ROOT)/deps/lwip/src/include

SOCKSRV_LIBRARY := icpsocket_server
SOCKSRV_TARGET := $(SOCKSRV_LIB_DIR)/lib$(SOCKSRV_LIBRARY).a

-include $(SOCKSRV_OBJECTS:.o=.d)

ICP_LDLIBS += -l$(SOCKSRV_LIBRARY) $(SOCKSRV_LDLIBS)

###
# Build rules
###
$(SOCKSRV_OBJECTS): | $(SOCKSRV_DEPENDS)

$(SOCKSRV_OBJ_DIR)/%.o: $(SOCKSRV_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(SOCKSRV_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(SOCKSRV_OBJ_DIR)/%.o: $(SOCKSRV_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(SOCKSRV_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(SOCKSRV_TARGET): $(SOCKSRV_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(SOCKSRV_OBJECTS))

.PHONY: socket_server
socket_server: $(SOCKSRV_TARGET)

.PHONY: clean_socket_server
clean_socket_server:
	@rm -rf $(SOCKSRV_OBJ_DIR) $(SOCKSRV_TARGET)
clean: clean_socket_server
