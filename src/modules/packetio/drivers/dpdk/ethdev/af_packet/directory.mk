
PIO_DRIVER_SOURCES += \
    ethdev/af_packet/rte_eth_af_packet.c

$(PIO_OBJ_DIR)/drivers/dpdk/ethdev/af_packet/rte_eth_af_packet.o: OP_CPPFLAGS += -DALLOW_INTERNAL_API -DALLOW_EXPERIMENTAL_API
