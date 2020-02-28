PIO_DEPENDS += dpdk timesync spirent_pga

PIO_DRIVER_SOURCES += \
	arg_parser.cpp \
	arg_parser_register.c \
	driver_factory.cpp \
	eal.cpp \
	flow_filter.cpp \
	mac_filter.cpp \
	mbuf_signature.cpp \
	port_filter.cpp \
	port_signature_decoder.cpp \
	port_signature_decoder_utils.cpp \
	port_timestamper.cpp \
	queue_utils.cpp \
	topology_utils.cpp \
	model/physical_port.cpp \
	model/port_info.cpp

# Needed to use the "experimental" API for DPDK's dynamic mbufs
$(PIO_OBJ_DIR)/drivers/dpdk/mbuf_signature.o: OP_CPPFLAGS += -Wno-deprecated-declarations
