#
# Makefile component to build the BPF code
#
BPF_DEPENDS += packetio libpcap sljit

BPF_TEST_DEPENDS += packetio_test libpcap sljit

BPF_SOURCES += \
	bpf.cpp \
	bpf_build.cpp \
	bpf_parse.cpp \
	bpf_sink.cpp

BPF_TEST_SOURCES += \
	bpf.cpp \
	bpf_build.cpp \
	bpf_parse.cpp \
	bpf_sink.cpp

include $(BPF_SRC_DIR)/bsd/directory.mk
