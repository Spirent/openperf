REGURGITATE_SOURCES += \
	benchmark.cpp \
	functions.cpp

ifeq ($(ARCH),x86_64)
	REGURGITATE_SOURCES += instruction_set_x86.cpp
else ifeq ($(ARCH),aarch64)
	REGURGITATE_SOURCES += instruction_set_aarch64.cpp
endif

include $(REGURGITATE_SRC_DIR)/ispc/directory.mk
