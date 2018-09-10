ICP_DPDK_TARGET ?= default-linuxicp-clang

PIO_DEPENDS += dpdk

PIO_DRIVER_SOURCES += \
	dpdk_init.cpp \
	dpdk_register.c

PIO_LIBS += -lcap
