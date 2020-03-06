#
# Makefile component to build Packet Generator code
#

PG_DEPENDS += api framework versions

PG_SOURCES += \
	handler.cpp \
	init.cpp

PG_VERSIONED_FILES := init.cpp
PG_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(PG_VERSIONED_FILES),$(PG_SOURCES)),$(PG_OBJ_DIR))

$(PG_OBJ_DIR)/init.o: $(PG_UNVERSIONED_OBJECTS)
$(PG_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
