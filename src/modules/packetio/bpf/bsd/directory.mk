PIO_DEPENDS += libpcap sljit

PIOTEST_DEPENDS += libpcap sljit

OP_CPPFLAGS += -DOPENPERF_NETBSD_BPF

PIO_SOURCES += \
	bpf/bsd/bpfjit.c \
	bpf/bsd/bpf_filter.c

PIOTEST_SOURCES += \
	bpf/bsd/bpfjit.c \
	bpf/bsd/bpf_filter.c
