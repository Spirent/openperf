# Common components for Spirent PGA library

PGA_SOURCES += \
	common/checksum.cpp \
	common/fill.cpp \
	common/signature.cpp \
	common/verify.cpp

PGA_INC_DIR += $(PGA_SRC_DIR)/common/$(ARCH)
