#
# Makefile component to build Analyzer code
#

PA_DEPENDS += \
	base_n \
	immer \
	packet_bpf \
	packet_protocol \
	packet_statistics \
	packetio \
	spirent_pga \
	timesync

PA_SOURCES += \
	api_transmogrify.cpp \
	handler.cpp \
	init.cpp \
	recycle_impl.cpp \
	server.cpp \
	sink.cpp \
	sink_transmogrify.cpp \
	statistics/counter_factory.cpp \
	statistics/utils.cpp \
	statistics/flow/counters.cpp \
	statistics/flow/header.cpp \
	statistics/flow/header_utils.cpp \
	statistics/flow/header_view.cpp \
	utils.cpp

$(PA_OBJ_DIR)/api_transmogrify.o: OP_CXXFLAGS += -Wno-unused-parameter
$(PA_OBJ_DIR)/handler.o: OP_CXXFLAGS += -Wno-unused-parameter
$(PA_OBJ_DIR)/init.o: OP_CXXFLAGS += -Wno-unused-parameter
$(PA_OBJ_DIR)/server.o: OP_CXXFLAGS += -Wno-unused-parameter
$(PA_OBJ_DIR)/sink.o: OP_CXXFLAGS += -Wno-unused-parameter

PA_VERSIONED_FILES := init.cpp
PA_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(PA_VERSIONED_FILES),$(PA_SOURCES)),$(PA_OBJ_DIR))

$(PA_OBJ_DIR)/init.o: $(PA_UNVERSIONED_OBJECTS)
$(PA_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

PA_TEST_DEPENDS += packet_protocol spirent_pga timesync_test

PA_TEST_SOURCES += \
	statistics/flow/header.cpp \
	statistics/flow/header_utils.cpp \
	statistics/flow/header_view.cpp
