#
# Makefile component for API code
#

API_DEPENDS += expected pistache json versions

API_SOURCES += \
	api_config_file_resources.cpp \
	api_init.cpp \
	api_internal_client.cpp \
	api_module_info.cpp \
	api_register.c \
	api_utils.cpp \
	api_version.cpp

API_VERSIONED_FILES := api_version.cpp api_init.cpp
API_VERSIONED_OBJECTS := $(call op_generate_objects,$(API_VERSIONED_FILES),$(API_OBJ_DIR))
API_UNVERSIONED_OBJECTS =\
	$(call op_generate_objects,$(filter-out $(API_VERSIONED_FILES),$(API_SOURCES)),$(API_OBJ_DIR))

$(API_VERSIONED_OBJECTS): $(API_UNVERSIONED_OBJECTS)
$(API_VERSIONED_OBJECTS): OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \
	-DBUILD_VERSION="\"$(GIT_VERSION)\""

API_TEST_DEPENDS += expected pistache

API_TEST_SOURCES += \
	api_config_file_resources.cpp \
	api_init.cpp \
	api_internal_client.cpp \
	api_utils.cpp

API_UNVERSIONED_TEST_OBJECTS =\
	$(call op_generate_objects,$(filter-out $(API_VERSIONED_FILES),$(API_TEST_SOURCES)),$(API_TEST_OBJ_DIR))

$(API_TEST_OBJ_DIR)/api_init.o: $(API_UNVERSIONED_TEST_OBJECTS)
$(API_TEST_OBJ_DIR)/api_init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
