#
# Makefile component for Memory Generator module
#

TVLP_DEPENDS += api framework pistache swagger_model json versions timesync

TVLP_SOURCES += \
	init.cpp \
	handler.cpp \
	server.cpp \
	generator.cpp

TVLP_VERSIONED_FILES := init.cpp
TVLP_UNVERSIONED_OBJECTS := \
	$(call op_generate_objects,$(filter-out $(TVLP_VERSIONED_FILES),$(TVLP_SOURCES)),$(TVLP_OBJ_DIR))

$(TVLP_OBJ_DIR)/init.o: $(TVLP_UNVERSIONED_OBJECTS)
$(TVLP_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
