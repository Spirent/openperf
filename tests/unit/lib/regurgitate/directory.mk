TEST_DEPENDS += regurgitate digestible

TEST_SOURCES += \
	lib/regurgitate/test_benchmark.cpp \
	lib/regurgitate/test_init.cpp \
	lib/regurgitate/test_merge.cpp \
	lib/regurgitate/test_problems.cpp \
	lib/regurgitate/test_sort.cpp

ifeq ($(ARCH),x86_64)
	TEST_SOURCES += lib/regurgitate/test_api_x86.cpp
else ifeq ($(ARCH),aarch64)
	TEST_SOURCES += lib/regurgitate/test_api_aarch64.cpp
endif

# We need to use the same build defines as the library in order to
# use some of our test functions
$(TEST_OBJ_DIR)/lib/regurgitate/%.o: TEST_FLAGS += $(addprefix -D,$(REGURGITATE_DEFINES))
