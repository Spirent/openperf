#
# Makefile component to specify block code
#

BLOCK_DEPENDS += api expected framework json pistache swagger_model timesync versions

BLOCK_SOURCES += \
	api_transmogrify.cpp \
	file_stack.cpp \
	device_stack.cpp \
	block_generator.cpp \
	generator_stack.cpp \
	init.cpp \
	handler.cpp \
	server.cpp \
	pattern_generator.cpp \
	task.cpp

include $(BLOCK_SRC_DIR)/models/directory.mk

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
