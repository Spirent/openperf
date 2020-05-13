#
# Makefile component to specify cpu code
#

CPU_DEPENDS += api expected framework json pistache swagger_model timesync versions spirent_pga

CPU_SOURCES += \
	api_transmogrify.cpp \
	init.cpp \
	handler.cpp \
	server.cpp \
	generator.cpp \
	generator_stack.cpp \
	task_cpu.cpp \
	cpu_info.cpp

ifeq ($(PLATFORM), linux)
	CPU_SOURCES += cpu_stats_linux.cpp
else ifneq ($(PLATFORM), osv)
	CPU_SOURCES += cpu_stats.cpp
endif

include $(PGA_SRC_DIR)/ispc/directory.mk

CPU_VERSIONED_FILES := init.cpp
CPU_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(CPU_VERSIONED_FILES),$(CPU_SOURCES)),$(CPU_OBJ_DIR))

$(CPU_OBJ_DIR)/init.o: $(CPU_UNVERSIONED_FILES)
$(CPU_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \
