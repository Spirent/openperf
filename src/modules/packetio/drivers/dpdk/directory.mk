PIO_DEPENDS += dpdk timesync spirent_pga

OP_PACKETIO_DPDK_PROCESS_TYPE ?= primary

PIO_DRIVER_SOURCES += \
	arg_parser.cpp \
	arg_parser_register.c \
	mbuf_metadata.cpp \
	port_info.cpp \
	port/checksum_calculator.cpp \
	port/filter.cpp \
	port/flow_filter.cpp \
	port/mac_filter.cpp \
	port/net_ring_fixup.cpp \
	port/packet_type_decoder.cpp \
	port/prbs_error_detector.cpp \
	port/rss_hasher.cpp \
	port/signature_decoder.cpp \
	port/signature_encoder.cpp \
	port/signature_payload_filler.cpp \
	port/signature_utils.cpp \
	port/timestamper.cpp \
	queue_utils.cpp \
	quirks.cpp \
	topology_utils.cpp

# ALLOW_EXPERIMENTAL_API is required for rte_lcore_cpuset()
$(PIO_OBJ_DIR)/drivers/dpdk/topology_utils.o: OP_CPPFLAGS += -DALLOW_EXPERIMENTAL_API

ifeq ($(OP_PACKETIO_DPDK_PROCESS_TYPE),primary)
	PIO_DRIVER_SOURCES += \
		primary/arg_parser.cpp \
		primary/arg_parser_register.c \
		primary/driver_factory.cpp \
		primary/eal_process.cpp \
		primary/port_info.cpp \
		primary/physical_port.cpp \
		primary/queue_utils.cpp \
		primary/topology_utils.cpp \
		primary/utils.cpp

else ifeq ($(OP_PACKETIO_DPDK_PROCESS_TYPE),secondary)
	PIO_DRIVER_SOURCES += \
		secondary/arg_parser.cpp \
		secondary/arg_parser_register.c \
		secondary/driver_factory.cpp \
		secondary/eal_process.cpp \
		secondary/port_info.cpp \
		secondary/queue_utils.cpp \
		secondary/topology_utils.cpp
else
	$(error unsupported DPDK process type $(OP_PACKETIO_DPDK_PROCESS_TYPE))
endif

include $(PIO_DRIVER_DIR)/ethdev/directory.mk
