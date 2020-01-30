#
# Makefile component for timesync unit tests
#

TEST_DEPENDS += timesync_test

TEST_SOURCES += \
	modules/timesync/test_bintime.cpp \
	modules/timesync/test_clock.cpp \
	modules/timesync/test_history.cpp
