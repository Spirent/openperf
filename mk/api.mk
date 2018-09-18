#
# Makefile component for API infrastructure
#

API_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(API_REQ_VARS))

API_SOURCES :=
API_INCLUDES :=
API_DEPENDS :=
API_LIBS :=

API_SRC_DIR := $(ICP_ROOT)/src/api
API_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/api
API_LIB_DIR := $(ICP_BUILD_ROOT)/api

include $(API_SRC_DIR)/module.mk

API_OBJECTS += $(patsubst %, $(API_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(API_SOURCES))))
API_OBJECTS += $(patsubst %, $(API_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(API_SOURCES))))

API_TARGET := $(API_LIB_DIR)/libapi.a

ICP_INC_DIRS += $(addprefix $(API_SRC_DIR)/,$(API_INCLUDES))
ICP_LIB_DIRS += $(API_LIB_DIR)
ICP_LDLIBS += -Wl,--whole-archive -lapi -Wl,--no-whole-archive

# Load dependencies
$(foreach DEP,$(API_DEPENDS),$(eval include $(ICP_ROOT)/mk/$(DEP).mk))
-include $(API_OBJECTS:.o=.d)

###
# Build rules
###
$(API_OBJ_DIR)/%.o: $(API_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $<)

$(API_OBJ_DIR)/%.o: $(API_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $<)

$(API_OBJECTS): | $(API_DEPENDS)

$(API_TARGET): $(API_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(API_OBJECTS))

.PHONY: api
api: $(API_TARGET)

.PHONY: clean_api
clean_api:
	@rm -rf $(API_OBJ_DIR) $(API_LIB_DIR)
clean: clean_api
