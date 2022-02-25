#
# Makefile component for Memory Generator module
#

MEMORY_DEPENDS += api framework json pistache range_v3 swagger_model timesync versions

MEMORY_SOURCES += \
	arg_parser.cpp \
	api_strings.cpp \
	api_transmogrify.cpp \
	generator/buffer.cpp \
	generator/coordinator.cpp \
	generator/worker.cpp \
	generator/worker_client.cpp \
	generator/worker_transmogrify.cpp \
	handler.cpp \
	init.cpp \
	memory_options.c \
	memory_transmogrify.cpp \
	server.cpp \
	utils.cpp

MEMORY_VERSIONED_FILES := init.cpp
MEMORY_UNVERSIONED_OBJECTS := \
	$(call op_generate_objects,$(filter-out $(MEMORY_VERSIONED_FILES),$(MEMORY_SOURCES)),$(MEMORY_OBJ_DIR))

$(MEMORY_OBJ_DIR)/init.o: $(MEMORY_UNVERSIONED_OBJECTS)
$(MEMORY_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

MEMORY_TEST_DEPENDS +=

MEMORY_TEST_SOURCES += \
	generator/buffer.cpp \
