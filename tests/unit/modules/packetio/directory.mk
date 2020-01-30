#
# Makefile component for packetio unit tests
#

TEST_DEPENDS += packetio_test

TEST_SOURCES += \
	modules/packetio/test_forwarding_table.cpp \
	modules/packetio/test_recycle.cpp \
	modules/packetio/test_transmit_table.cpp
