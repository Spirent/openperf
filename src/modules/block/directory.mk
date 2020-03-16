#
# Makefile component to specify timesync code
#

BLOCK_DEPENDS += versions

BLOCK_SOURCES += \
	file_stack.cpp \
	device_stack.cpp \
	block_generator.cpp \
	generator_stack.cpp \
	init.cpp \
	handler.cpp \
	server.cpp \
	worker.cpp

include $(BLOCK_SRC_DIR)/models/directory.mk

BLOCK_VERSIONED_FILES := init.cpp
BLOCK_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(BLOCK_VERSIONED_FILES),$(BLOCK_SOURCES)),$(BLOCK_OBJ_DIR))

$(BLOCK_OBJ_DIR)/init.o: $(BLOCK_UNVERSIONED_FILES)
$(BLOCK_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \
