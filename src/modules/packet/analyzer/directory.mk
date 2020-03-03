#
# Makefile component to build Analyzer code
#

PA_DEPENDS += immer packetio spirent_pga timesync

PA_SOURCES += \
	api_transmogrify.cpp \
	handler.cpp \
	init.cpp \
	recycle_impl.cpp \
	server.cpp \
	sink.cpp \
	sink_transmogrify.cpp \
	statistics/common.cpp \
	statistics/flow/factory.cpp \
	statistics/flow/counters.cpp \
	statistics/flow/utils.cpp \
	statistics/protocol/factory.cpp \
	statistics/protocol/counters.cpp \
	statistics/protocol/utils.cpp \
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
