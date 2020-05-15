SWAGGER_DEPENDS += json
SWAGGER_INCLUDES += v1/model

OP_CPPFLAGS += -Wno-unused-parameter


# Just suck up every cpp file in the model directory properly formatted
# for our generic build rules
SWAGGER_SOURCES += $(patsubst $(SWAGGER_SRC_DIR)/%,%,$(wildcard $(SWAGGER_SRC_DIR)/v1/model/*.cpp))
