# Makefile component for packet unit tests

TEST_DEPENDS += packet

TEST_SOURCES += \
	lib/packet/test_endian_fields.cpp \
	lib/packet/test_ipv4_address.cpp \
	lib/packet/test_ipv6_address.cpp \
	lib/packet/test_mac_address.cpp

ifeq ($(ARCH),x86_64)
	TEST_SOURCES += lib/packet/test_endian_order_x86.cpp
endif
