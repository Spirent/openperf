OP_INC_DIRS += $(OP_ROOT)/src/modules

TEST_DEPENDS += packet_generator_test

TEST_SOURCES += \
	modules/packet/generator/test_modifiers.cpp \
	modules/packet/generator/test_header_utils.cpp \
	modules/packet/generator/test_packet_template.cpp \
	modules/packet/generator/test_sequence.cpp
