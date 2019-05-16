#
# Makefile component for API code
#

API_DEPENDS += pistache json

API_SOURCES += \
	api_config_file_resources.cpp \
	api_init.cpp \
	api_internal_client.cpp \
	api_module_info.cpp \
	api_register.c \
	api_utils.cpp \
	api_version.cpp

.PHONY: $(API_SRC_DIR)/api_init.cpp
$(API_OBJ_DIR)/api_init.o: ICP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

.PHONY: $(API_SRC_DIR)/api_version.cpp
$(API_OBJ_DIR)/api_version.o: ICP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \
	-DBUILD_VERSION="\"$(GIT_VERSION)\""

API_TEST_DEPENDS += pistache

API_TEST_SOURCES += \
	api_config_file_resources.cpp \
	api_init.cpp \
	api_internal_client.cpp \
	api_utils.cpp

.PHONY: $(API_TEST_SRC_DIR)/api_init.cpp
$(API_TEST_OBJ_DIR)/api_init.o: ICP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
