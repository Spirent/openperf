OP_INC_DIRS += $(OP_ROOT)/src/modules

TEST_DEPENDS += packet_analyzer_test

TEST_SOURCES += \
	modules/packet/analyzer/test_flow_counters.cpp \
	modules/packet/analyzer/test_flow_headers.cpp \
	modules/packet/analyzer/test_protocol_counters.cpp
