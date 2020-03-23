#
# Makefile component for Memory Generator module
#

MEMGEN_DEPENDS += swagger_model json versions

MEMGEN_SOURCES += \
	init.cpp \
	handler.cpp \
	server.cpp \
	generator_collection.cpp \
	info.cpp \
	worker.cpp \
	generator.cpp \
	generator_config.cpp \
	task_memory.cpp \
	task_memory_read.cpp \
	task_memory_write.cpp

MEMGEN_VERSIONED_FILES := init.cpp
MEMGEN_UNVERSIONED_OBJECTS := \
	$(call op_generate_objects,$(filter-out $(MEMGEN_VERSIONED_FILES),$(MEMGEN_SOURCES)),$(MEMGEN_OBJ_DIR))

$(MEMGEN_OBJ_DIR)/init.o: $(MEMGEN_UNVERSIONED_OBJECTS)
$(MEMGEN_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
