#
# Makefile component for API code
#

API_DEPENDS += pistache json

API_SOURCES += \
	api_init.cpp \
	api_register.c \
	api_version.cpp

# Tag the build with the git commit version
GIT_COMMIT := $(shell git rev-parse --verify HEAD)
# If working tree is dirty, append dirty flag
ifneq ($(strip $(shell git status --porcelain 2>/dev/null)),)
	GIT_COMMIT := $(GIT_COMMIT)-dirty
endif

# Use the Jenkins build number, if present, to tag the build.
ifeq ($(BUILD_NUMBER),)
BUILD_NUMBER = 0
$(warning No BUILD_NUMBER defined in environment, using 0)
endif

.PHONY: $(API_SRC_DIR)/api_version.cpp
$(API_OBJ_DIR)/api_version.o: TIMESTAMP := $(shell date -u +%Y-%m-%dT%H:%M:%S%Z)
$(API_OBJ_DIR)/api_version.o: ICP_CPPFLAGS += \
	-DVERSION_COMMIT="\"$(GIT_COMMIT)\"" \
	-DVERSION_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DVERSION_TIMESTAMP="\"$(TIMESTAMP)\""
