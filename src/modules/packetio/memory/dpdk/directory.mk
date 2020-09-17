PIO_MEMORY_SOURCES += \
	memp.c \
	packet_buffer.cpp \
	pbuf.c \
	pbuf_utils.c

ifeq ($(OP_PACKETIO_DPDK_PROCESS_TYPE),primary)
	PIO_MEMORY_SOURCES += \
		primary/memory.c \
		primary/mempool.cpp \
		primary/pool_allocator.cpp

else ifeq ($(OP_PACKETIO_DPDK_PROCESS_TYPE),secondary)
	PIO_MEMORY_SOURCES += \
		secondary/memory.cpp \
		secondary/mempool.cpp \
		secondary/option_register.c
else
	$(error unsupported DPDK process type $(OP_PACKETIO_DPDK_PROCESS_TYPE))
endif
