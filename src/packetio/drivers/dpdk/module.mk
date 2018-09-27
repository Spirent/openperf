ICP_DPDK_TARGET ?= default-linuxicp-clang

PIO_DEPENDS += dpdk

PIO_DRIVER_SOURCES += \
	dpdk_init.cpp \
	dpdk_register.c \
	port_service_impl.cpp

PIO_LIBS += -lcap
