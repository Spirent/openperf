#
# Makefile component for packetio unit tests
#

TEST_DEPENDS += packetio_test

TEST_SOURCES += \
	modules/packetio/mock_packet_buffer.cpp \
	modules/packetio/test_forwarding_table.cpp \
	modules/packetio/test_transmit_table.cpp
