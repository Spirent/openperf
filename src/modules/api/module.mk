#
# Makefile component for API code
#

API_DEPENDS += pistache json

API_SOURCES += \
	api_init.cpp \
	api_internal_client.cpp \
	api_module_info.cpp \
	api_register.c \
	api_version.cpp

API_TEST_DEPENDS += pistache

API_TEST_SOURCES += \
	api_internal_client.cpp

.PHONY: $(API_SRC_DIR)/api_version.cpp
$(API_OBJ_DIR)/api_version.o: ICP_CPPFLAGS += \
	-DVERSION_COMMIT="\"$(GIT_COMMIT)\"" \
	-DVERSION_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DVERSION_TIMESTAMP="\"$(TIMESTAMP)\""

.PHONY: $(API_SRC_DIR)/api_init.cpp
$(API_OBJ_DIR)/api_init.o: ICP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
