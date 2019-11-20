$(call op_check_vars, OP_PACKETIO_DRIVER)

PIO_WORKER_SOURCES :=
include $(PIO_SRC_DIR)/workers/$(OP_PACKETIO_DRIVER)/directory.mk

PIO_SOURCES += $(addprefix workers/$(OP_PACKETIO_DRIVER)/,$(PIO_WORKER_SOURCES))
