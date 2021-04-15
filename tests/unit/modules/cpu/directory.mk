#
# Makefile component for cpu generator unit tests
#

TEST_DEPENDS += cpu_test
TEST_SOURCES += \
	modules/cpu/test_ispc_target.cpp

ifeq ($(ARCH),x86_64)
	TEST_SOURCES += \
		modules/cpu/test_ispc_target_x86.cpp
endif

ifeq ($(ARCH),aarch64)
	TEST_SOURCES ++ \
		modules/cpu/test_ispc_target_aarch64.cpp
endif

$(TEST_OBJ_DIR)/modules/cpu/%.o: TEST_FLAGS += $(addprefix -D,$(CPU_TEST_DEFINES))
