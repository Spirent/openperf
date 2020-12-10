#
# Makefile stub for Spirent PGA library
#

PGA_SOURCES += \
	api.cpp \
	benchmark.cpp \
	functions.cpp

ifeq ($(ARCH),x86_64)
	PGA_SOURCES += instruction_set_x86.cpp
else ifeq ($(ARCH),aarch64)
	PGA_SOURCES += instruction_set_aarch64.cpp
endif

include $(PGA_SRC_DIR)/common/directory.mk
include $(PGA_SRC_DIR)/ispc/directory.mk
include $(PGA_SRC_DIR)/scalar/directory.mk
