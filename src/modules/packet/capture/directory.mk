#
# Makefile component to build Capture code
#

CAP_DEPENDS += immer packetio spirent_pga timesync

CAP_SOURCES += \
        api_transmogrify.cpp \
        capture_buffer.cpp \
        handler.cpp \
        init.cpp \
        pcap_handler.cpp \
        pcap_writer.cpp \
        pistache_utils.cpp \
    	server.cpp \
    	sink.cpp \
        sink_transmogrify.cpp \
    	utils.cpp

$(CAP_OBJ_DIR)/api_transmogrify.o: OP_CXXFLAGS += -Wno-unused-parameter
$(CAP_OBJ_DIR)/handler.o: OP_CXXFLAGS += -Wno-unused-parameter
$(CAP_OBJ_DIR)/init.o: OP_CXXFLAGS += -Wno-unused-parameter
$(CAP_OBJ_DIR)/server.o: OP_CXXFLAGS += -Wno-unused-parameter
$(CAP_OBJ_DIR)/sink.o: OP_CXXFLAGS += -Wno-unused-parameter

CAP_VERSIONED_FILES := init.cpp
CAP_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(CAP_VERSIONED_FILES),$(CAP_SOURCES)),$(CAP_OBJ_DIR))

$(CAP_OBJ_DIR)/init.o: $(CAP_UNVERSIONED_OBJECTS)
$(CAP_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

CAP_TEST_DEPENDS += digestible expected framework

CAP_TEST_SOURCES += \
        capture_buffer.cpp

