OP_CPPFLAGS += -DOPENPERF_NETBSD_BPF

BPF_SOURCES += \
	bsd/bpfjit.c \
	bsd/bpf_filter.c

BPF_TEST_SOURCES += \
	bsd/bpfjit.c \
	bsd/bpf_filter.c
