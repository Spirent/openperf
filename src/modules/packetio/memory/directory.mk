$(call op_check_vars, OP_PACKETIO_DRIVER)

PIO_MEMORY_DIR := $(PIO_SRC_DIR)/memory/$(OP_PACKETIO_DRIVER)

include $(PIO_MEMORY_DIR)/directory.mk

PIO_SOURCES += $(addprefix memory/$(OP_PACKETIO_DRIVER)/,$(PIO_MEMORY_SOURCES))
