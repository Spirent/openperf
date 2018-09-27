#
# Makefile component for swagger models
#

SWAGGER_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(SWAGGER_REQ_VARS))

SWAGGER_SOURCES :=
SWAGGER_INCLUDES :=
SWAGGER_DEPENDS :=
SWAGGER_LIBS :=

SWAGGER_SRC_DIR := $(ICP_ROOT)/src/swagger
SWAGGER_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/swagger
SWAGGER_LIB_DIR := $(ICP_BUILD_ROOT)/lib

include $(SWAGGER_SRC_DIR)/module.mk

SWAGGER_OBJECTS += $(patsubst %, $(SWAGGER_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(SWAGGER_SOURCES)))

SWAGGER_TARGET := $(SWAGGER_LIB_DIR)/libswagger_v1_model.a

ICP_INC_DIRS += $(addprefix $(SWAGGER_SRC_DIR)/,$(SWAGGER_INCLUDES))
ICP_LIB_DIRS += $(SWAGGER_LIB_DIR)
ICP_LDLIBS += -lswagger_v1_model

# Load dependencies
$(foreach DEP,$(SWAGGER_DEPENDS),$(eval include $(ICP_ROOT)/mk/$(DEP).mk))
-include $(SWAGGER_OBJECTS:.o=.d)

###
# Build rules
###
$(SWAGGER_OBJ_DIR)/%.o: $(SWAGGER_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $<)

$(SWAGGER_OBJECTS): | $(SWAGGER_DEPENDS)

$(SWAGGER_TARGET): $(SWAGGER_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(SWAGGER_OBJECTS))

.PHONY: swagger_model
swagger_model: $(SWAGGER_TARGET)

.PHONY: clean_swagger_model
clean_swagger_model:
	@rm -rf $(SWAGGER_OBJ_DIR) $(SWAGGER_TARGET)
clean: clean_swagger_model
