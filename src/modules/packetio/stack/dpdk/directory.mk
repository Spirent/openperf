PIO_INCLUDES += stack/dpdk/include

PIO_STACK_SOURCES += \
	lwip.cpp \
	gso_utils.cpp \
	net_interface.cpp \
	net_interface_rx.cpp \
	netif_utils.cpp \
	netif_wrapper.cpp \
	offload_utils.cpp \
	stack_factory.cpp \
	stack_msg_handler.cpp \
	stack_timeout_handler.cpp \
	sys_arch.cpp \
	sys_mailbox.cpp
