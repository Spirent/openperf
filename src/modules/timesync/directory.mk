#
# Makefile component to specify timesync code
#

TS_DEPENDS += api digestible expected framework json pistache versions

TS_SOURCES += \
	api_transmogrify.cpp \
	chrono.cpp \
	clock.cpp \
	counter_system.cpp \
	init.cpp \
	handler.cpp \
	history.cpp \
	ntp.cpp \
	server.cpp \
	socket.cpp

ifeq ($(ARCH), x86_64)
	TS_SOURCES += counter_tsc.cpp
endif

TS_VERSIONED_FILES := init.cpp
TS_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(TS_VERSIONED_FILES),$(TS_SOURCES)),$(TS_OBJ_DIR))

$(TS_OBJ_DIR)/init.o: $(TS_UNVERSIONED_FILES)
$(TS_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\"" \

TS_TEST_DEPENDS += digestible expected

TS_TEST_SOURCES += \
	clock.cpp \
	counter_system.cpp \
	history.cpp
