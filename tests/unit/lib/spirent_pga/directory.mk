# Makefile component for spirent_pga unit tests

TEST_SOURCES += \
	lib/spirent_pga/test_init.cpp \
	lib/spirent_pga/test_fills.cpp \
	lib/spirent_pga/test_prbs.cpp \
	lib/spirent_pga/test_signatures.cpp

TEST_DEPENDS += spirent_pga

# We need to use the same build defines as the library in order to
# use some of our test functions
$(TEST_OBJ_DIR)/lib/spirent_pga/%.o: TEST_FLAGS += $(addprefix -D,$(PGA_DEFINES))
