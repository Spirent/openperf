#
# Makefile component for socket unit tests
#

TEST_DEPENDS += socket_test

TEST_SOURCES += \
	modules/socket/test_circular_buffer.cpp \
	modules/socket/test_event_queue.cpp
