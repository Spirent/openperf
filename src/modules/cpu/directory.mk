#
# Makefile component to specify cpu code
#

CPU_DEPENDS += api expected framework json pistache swagger_model \
	timesync versions

CPU_SOURCES += \
	api_converters.cpp \
	api_transmogrify.cpp \
	cpu_stat.cpp \
	generator.cpp \
	generator_stack.cpp \
	handler.cpp \
	init.cpp \
	server.cpp \
	task_cpu.cpp \
	instruction_set_x86.cpp

ifeq ($(PLATFORM), linux)
	CPU_SOURCES += cpu_linux.cpp
endif

CPU_VERSIONED_FILES := init.cpp
CPU_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(CPU_VERSIONED_FILES),$(CPU_SOURCES)),$(CPU_OBJ_DIR))

$(CPU_OBJ_DIR)/init.o: $(CPU_UNVERSIONED_FILES)
$(CPU_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \
