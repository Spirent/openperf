PP_DEPENDS += packet swagger_model

# Pull in all automatically generated sources and format them for our build rules
PP_SOURCES += $(patsubst $(PP_SRC_DIR)/%,%,$(wildcard $(PP_SRC_DIR)/transmogrify/*.cpp))
