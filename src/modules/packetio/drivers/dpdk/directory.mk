ifeq ($(MODE),debug)
ICP_DPDK_TARGET ?= default-linuxicp-clang-debug
else
ICP_DPDK_TARGET ?= default-linuxicp-clang
endif

PIO_DEPENDS += dpdk

PIO_DRIVER_SOURCES += \
	arg_parser.cpp \
	arg_parser_register.c \
	driver.cpp \
	eal.cpp \
	epoll_poller.cpp \
	queue_utils.cpp \
	rx_queue.cpp \
	topology_utils.cpp \
	tx_queue.cpp \
	vif_map_impl.cpp \
	worker.cpp \
	worker_client.cpp \
	zmq_socket.cpp \
	model/physical_port.cpp \
	model/port_info.cpp

PIO_LIBS += -lcap
