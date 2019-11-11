ifeq ($(MODE),debug)
ICP_DPDK_TARGET ?= default-linuxicp-clang-debug
else
ICP_DPDK_TARGET ?= default-linuxicp-clang
endif

PIO_DEPENDS += dpdk

PIO_DRIVER_SOURCES += \
	arg_parser.cpp \
	arg_parser_register.c \
	driver_factory.cpp \
	eal.cpp \
	flow_filter.cpp \
	mac_filter.cpp \
	port_filter.cpp \
	queue_utils.cpp \
	topology_utils.cpp \
	model/physical_port.cpp \
	model/port_info.cpp
