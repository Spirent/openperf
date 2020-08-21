SWAGGER_DEPENDS += json
SWAGGER_INCLUDES += v1/model

OP_CPPFLAGS += -Wno-unused-parameter


# Just suck up every cpp file in the model directory properly formatted
# for our generic build rules
SWAGGER_SOURCES += $(patsubst $(SWAGGER_SRC_DIR)/%,%,$(wildcard $(SWAGGER_SRC_DIR)/v1/model/*.cpp))
SWAGGER_SOURCES += $(patsubst $(SWAGGER_SRC_DIR)/%,%,$(wildcard $(SWAGGER_SRC_DIR)/converters/*.cpp))

# Custom types in swagger generate useless JSON translation functions with
# unused arguments.  Hence, we need to append a compiler flag to compile
# them without warnings
SWAGGER_CUSTOM_TYPE_OBJECTS = \
	BinaryString \
	Ipv4Address \
	Ipv6Address \
	MacAddress \
	TcpIpPort \
	PacketProtocolCustom

define add_swagger_build_flag
$(SWAGGER_OBJ_DIR)/v1/model/$(1).o: OP_CXXFLAGS += $(2)

endef

$(eval $(foreach obj,$(SWAGGER_CUSTOM_TYPE_OBJECTS),$(call add_swagger_build_flag,$(obj),-Wno-unused-parameter)))
