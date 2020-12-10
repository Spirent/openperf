# Makefile component for spirent_pga unit tests

TEST_DEPENDS += spirent_pga

TEST_SOURCES += \
	lib/spirent_pga/test_checksums.cpp \
	lib/spirent_pga/test_decode.cpp \
	lib/spirent_pga/test_init.cpp \
	lib/spirent_pga/test_fills.cpp \
	lib/spirent_pga/test_prbs.cpp \
	lib/spirent_pga/test_signatures.cpp \
	lib/spirent_pga/test_unpack_and_sum.cpp

ifeq ($(ARCH),x86_64)
	TEST_SOURCES += lib/spirent_pga/api_test_x86.cpp
else ifeq ($(ARCH),aarch64)
	TEST_SOURCES += lib/spirent_pga/api_test_aarch64.cpp
endif

# We need to use the same build defines as the library in order to
# use some of our test functions
$(TEST_OBJ_DIR)/lib/spirent_pga/%.o: TEST_FLAGS += $(addprefix -D,$(PGA_DEFINES))
