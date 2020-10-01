#
# Makefile component to specify block code
#

BLOCK_DEPENDS += api expected framework json pistache swagger_model timesync versions

BLOCK_SOURCES += \
	api_converters.cpp \
	api_transmogrify.cpp \
	block_generator.cpp \
	device_stack.cpp \
	file_stack.cpp \
	generator_stack.cpp \
	handler.cpp \
	init.cpp \
	pattern_generator.cpp \
	server.cpp \
	task.cpp \
	virtual_device.cpp \
	block_options.c

BLOCK_LDLIBS += -lrt

BLOCK_VERSIONED_FILES := init.cpp
BLOCK_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(BLOCK_VERSIONED_FILES),$(BLOCK_SOURCES)),$(BLOCK_OBJ_DIR))

$(BLOCK_OBJ_DIR)/init.o: $(BLOCK_UNVERSIONED_FILES)
$(BLOCK_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \

BLOCK_TEST_DEPENDS += digestible expected framework

BLOCK_TEST_SOURCES += \
	pattern_generator.cpp
