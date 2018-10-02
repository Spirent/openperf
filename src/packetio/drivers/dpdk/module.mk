ICP_DPDK_TARGET ?= default-linuxicp-clang

PIO_DEPENDS += dpdk tinyfsm

PIO_DRIVER_SOURCES += \
	eal.cpp \
	port_service_impl.cpp \
	register_options.c

PIO_LIBS += -lcap
