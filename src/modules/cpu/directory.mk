#
# Makefile component to specify cpu code
#

CPU_DEPENDS += \
	api \
	dynamic \
	expected \
	framework \
	timesync \
	versions

CPU_SOURCES += \
	api_strings.cpp \
	api_transmogrify.cpp \
	arg_parser.cpp \
	cpu_options.c \
	cpu_transmogrify.cpp \
	generator/coordinator.cpp \
	generator/ispc/matrix.ispc \
	generator/scalar/matrix.cpp \
	generator/worker.cpp \
	generator/worker_client.cpp \
	generator/worker_transmogrify.cpp \
	handler.cpp \
	init.cpp \
	server.cpp \
	utils.cpp

ifeq ($(ARCH),x86_64)
	CPU_SOURCES += \
		generator/instruction_set_x86.cpp
	CPU_TEST_SOURCES += generator/instruction_set_x86.cpp
endif

ifeq ($(ARCH),aarch64)
	CPU_SOURCES += \
		generator/instruction_set_aarch64.cpp
	CPU_TEST_SOURCES += generator/instruction_set_aarch64.cpp
endif

ifeq ($(PLATFORM),linux)
	CPU_SOURCES += generator/system_stats_linux.cpp
endif

CPU_VERSIONED_FILES := init.cpp
CPU_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(CPU_VERSIONED_FILES),$(CPU_SOURCES)),$(CPU_OBJ_DIR))

$(CPU_OBJ_DIR)/init.o: $(CPU_UNVERSIONED_FILES)
$(CPU_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

CPU_TEST_SOURCES += \
	generator/scalar/matrix.cpp \
	generator/ispc/matrix.ispc
