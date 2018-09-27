SWAGGER_DEPENDS += json
SWAGGER_INCLUDES += v1/model

include $(SWAGGER_SRC_DIR)/v1/model/module.mk

SWAGGER_SOURCES += $(addprefix v1/model/,$(MODEL_SOURCES))
