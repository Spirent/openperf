PIO_DEPENDS += dpdk timesync spirent_pga

PIO_DRIVER_SOURCES += \
	arg_parser.cpp \
	arg_parser_register.c \
	driver_factory.cpp \
	eal.cpp \
	mbuf_signature.cpp \
	model/physical_port.cpp \
	model/port_info.cpp \
	port/flow_filter.cpp \
	port/mac_filter.cpp \
	port/filter.cpp \
	port/packet_type_decoder.cpp \
	port/signature_decoder.cpp \
	port/signature_encoder.cpp \
	port/signature_utils.cpp \
	port/timestamper.cpp \
	queue_utils.cpp \
	topology_utils.cpp

# Needed to use the "experimental" API for DPDK's dynamic mbufs
$(PIO_OBJ_DIR)/drivers/dpdk/mbuf_signature.o: OP_CPPFLAGS += -Wno-deprecated-declarations
