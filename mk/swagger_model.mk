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

SWAGGER_OBJECTS := $(call icp_generate_objects,$(SWAGGER_SOURCES),$(SWAGGER_OBJ_DIR))

SWAGGER_TARGET := $(SWAGGER_LIB_DIR)/libswagger_v1_model.a

ICP_INC_DIRS += $(addprefix $(SWAGGER_SRC_DIR)/,$(SWAGGER_INCLUDES))
ICP_LIB_DIRS += $(SWAGGER_LIB_DIR)
ICP_LDLIBS += -lswagger_v1_model

# Load dependencies
-include $(SWAGGER_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(SWAGGER_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(SWAGGER_SOURCES),SWAGGER_SRC_DIR,SWAGGER_OBJ_DIR,SWAGGER_DEPENDS))
$(eval $(call icp_generate_clean_rules,swagger_model,SWAGGER_TARGET,SWAGGER_OBJECTS))

$(SWAGGER_TARGET): $(SWAGGER_OBJECTS)
	$(call icp_link_library,$@,$(SWAGGER_OBJECTS))

.PHONY: swagger_model
swagger_model: $(SWAGGER_TARGET)
