PIO_DEPENDS += dpdk timesync

PIO_DRIVER_SOURCES += \
	arg_parser.cpp \
	arg_parser_register.c \
	driver_factory.cpp \
	eal.cpp \
	flow_filter.cpp \
	mac_filter.cpp \
	port_filter.cpp \
	port_timestamper.cpp \
	queue_utils.cpp \
	topology_utils.cpp \
	model/physical_port.cpp \
	model/port_info.cpp
