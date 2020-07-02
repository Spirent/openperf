#
# Makefile component for Memory Generator module
#

MEMORY_DEPENDS += api framework pistache swagger_model json versions timesync

MEMORY_SOURCES += \
	api_transmogrify.cpp \
	init.cpp \
	handler.cpp \
	server.cpp \
	generator_collection.cpp \
	info.cpp \
	generator.cpp \
	memory_stat.cpp \
	task_memory.cpp \
	task_memory_read.cpp \
	task_memory_write.cpp

MEMORY_VERSIONED_FILES := init.cpp
MEMORY_UNVERSIONED_OBJECTS := \
	$(call op_generate_objects,$(filter-out $(MEMORY_VERSIONED_FILES),$(MEMORY_SOURCES)),$(MEMORY_OBJ_DIR))

$(MEMORY_OBJ_DIR)/init.o: $(MEMORY_UNVERSIONED_OBJECTS)
$(MEMORY_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

MEMORY_TEST_DEPENDS += framework timesync_test

MEMORY_TEST_SOURCES += \
	task_memory.cpp \
	generator.cpp \
	task_memory_read.cpp \
	task_memory_write.cpp \
	memory_stat.cpp
