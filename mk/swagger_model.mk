#
# Makefile component for swagger models
#

SWAGGER_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(SWAGGER_REQ_VARS))

SWAGGER_SOURCES :=
SWAGGER_INCLUDES :=
SWAGGER_DEPENDS :=
SWAGGER_LDOPTS := -fPIC

SWAGGER_SRC_DIR := $(OP_ROOT)/src/swagger
SWAGGER_OBJ_DIR := $(OP_BUILD_ROOT)/obj/swagger
SWAGGER_LIB_DIR := $(OP_BUILD_ROOT)/lib

include $(SWAGGER_SRC_DIR)/directory.mk

SWAGGER_OBJECTS := $(call op_generate_objects,$(SWAGGER_SOURCES),$(SWAGGER_OBJ_DIR))

SWAGGER_TARGET := $(SWAGGER_LIB_DIR)/libswagger_v1_model.a

OP_INC_DIRS += $(addprefix $(SWAGGER_SRC_DIR)/,$(SWAGGER_INCLUDES))
OP_LIB_DIRS += $(SWAGGER_LIB_DIR)
OP_LDLIBS += -lswagger_v1_model

# Load dependencies
-include $(SWAGGER_OBJECTS:.o=.d)
$(call op_include_dependencies,$(SWAGGER_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(SWAGGER_SOURCES),SWAGGER_SRC_DIR,SWAGGER_OBJ_DIR,SWAGGER_DEPENDS,SWAGGER_LDOPTS))
$(eval $(call op_generate_clean_rules,swagger_model,SWAGGER_TARGET,SWAGGER_OBJECTS))

$(SWAGGER_TARGET): $(SWAGGER_OBJECTS)
	$(call op_link_library,$@,$(SWAGGER_OBJECTS))

.PHONY: swagger_model
swagger_model: $(SWAGGER_TARGET)
