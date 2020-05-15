OP_INC_DIRS += $(OP_ROOT)/src/modules

TEST_DEPENDS += packetio_test packet_capture_test

TEST_SOURCES += \
	modules/packet/capture/test_capture_buffer.cpp \
