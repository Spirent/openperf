$(call icp_check_vars, ICP_PACKETIO_DRIVER)

PIO_MEMORY_DIR := $(PIO_SRC_DIR)/memory/$(ICP_PACKETIO_DRIVER)

include $(PIO_MEMORY_DIR)/directory.mk

PIO_SOURCES += $(addprefix memory/$(ICP_PACKETIO_DRIVER)/,$(PIO_MEMORY_SOURCES))
