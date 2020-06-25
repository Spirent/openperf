#
# Makefile component for packetio unit tests
#
OP_INC_DIRS += $(OP_ROOT)/src/modules

TEST_DEPENDS += packet_bpf_test packetio_test libpcap sljit

TEST_SOURCES += \
	modules/packet/bpf/test_bpf.cpp \
	modules/packet/bpf/test_bpf_parse.cpp
