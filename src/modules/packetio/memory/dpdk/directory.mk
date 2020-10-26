PIO_MEMORY_SOURCES += \
	packet_buffer.cpp

ifeq ($(OP_PACKETIO_DPDK_PROCESS_TYPE),primary)
	PIO_MEMORY_SOURCES += \
		primary/mempool.cpp \
		primary/pool_allocator.cpp
else ifeq ($(OP_PACKETIO_DPDK_PROCESS_TYPE),secondary)
	PIO_MEMORY_SOURCES += \
		secondary/mempool.cpp \
		secondary/option_register.c
else
	$(error unsupported DPDK process type $(OP_PACKETIO_DPDK_PROCESS_TYPE))
endif
