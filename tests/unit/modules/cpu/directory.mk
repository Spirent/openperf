#
# Makefile component for cpu generator unit tests
#

TEST_DEPENDS += cpu_test

TEST_SOURCES += \
	modules/cpu/test_ispc_target.cpp

$(TEST_OBJ_DIR)/modules/cpu/%.o: TEST_FLAGS += $(addprefix -D,$(CPU_TEST_DEFINES))
