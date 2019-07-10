$(call icp_check_vars, ICP_PACKETIO_DRIVER)

PIO_WORKER_SOURCES :=
include $(PIO_SRC_DIR)/workers/$(ICP_PACKETIO_DRIVER)/directory.mk

PIO_SOURCES += $(addprefix workers/$(ICP_PACKETIO_DRIVER)/,$(PIO_WORKER_SOURCES))
