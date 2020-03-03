OP_INC_DIRS += $(OP_ROOT)/src/modules

TEST_DEPENDS += timesync_test

TEST_SOURCES += \
	modules/packet/analyzer/test_flow_counters.cpp \
	modules/packet/analyzer/test_protocol_counters.cpp
