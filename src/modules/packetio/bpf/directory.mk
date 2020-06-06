PIO_SOURCES += \
	bpf/bpf.cpp \
	bpf/bpf_build.cpp \
	bpf/bpf_parse.cpp

PIOTEST_SOURCES += \
	bpf/bpf.cpp \
	bpf/bpf_build.cpp \
	bpf/bpf_parse.cpp

include $(PIO_SRC_DIR)/bpf/bsd/directory.mk


