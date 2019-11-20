$(call op_check_vars, OP_PACKETIO_DRIVER)

PIO_DRIVER_DIR := $(PIO_SRC_DIR)/drivers/$(OP_PACKETIO_DRIVER)

include $(PIO_DRIVER_DIR)/directory.mk

PIO_SOURCES += $(addprefix drivers/$(OP_PACKETIO_DRIVER)/,$(PIO_DRIVER_SOURCES))
