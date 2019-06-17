$(call icp_check_vars, ICP_PACKETIO_DRIVER)

PIO_DRIVER_DIR := $(PIO_SRC_DIR)/drivers/$(ICP_PACKETIO_DRIVER)

include $(PIO_DRIVER_DIR)/directory.mk

PIO_SOURCES += $(addprefix drivers/$(ICP_PACKETIO_DRIVER)/,$(PIO_DRIVER_SOURCES))
