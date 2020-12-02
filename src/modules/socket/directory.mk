SOCK_COMMON_SOURCES += \
	common.cpp \
	shared_segment.cpp \
	unix_socket.cpp

ifeq ($(PLATFORM),linux)
	SOCK_COMMON_SOURCES += process_control_linux.cpp
	SOCK_COMMON_LDLIBS += -lcap
endif

SOCK_COMMON_LDLIBS += -lrt
SOCK_COMMON_DEPENDS += expected

SOCKSRV_SOURCES += \
	server/api_handler.cpp \
	server/api_server.cpp \
	server/api_server_options.c \
	server/dgram_channel.cpp \
	server/icmp_socket.cpp \
	server/init.cpp \
	server/lwip_tcp_event.cpp \
	server/lwip_utils.cpp \
	server/pbuf_queue.cpp \
	server/pbuf_vec.cpp \
	server/raw_socket.cpp \
	server/socket_utils.cpp \
	server/stream_channel.cpp \
	server/tcp_socket.cpp \
	server/udp_socket.cpp

ifeq ($(PLATFORM),linux)
SOCKSRV_SOURCES += \
		server/packet_socket.cpp \
		server/socket_factory_linux.cpp
endif

SOCKSRV_DEPENDS += framework packet_stack versions socket_common

# Each channel object lives in shared memory and has both a client and server
# "version".  Each version contains the same members, but client's don't touch
# server data and vice versa.  Hence, this warning is safe to ignore.
$(SOCKSRV_OBJ_DIR)/server/dgram_channel.o: OP_CXXFLAGS += -Wno-unused-private-field
$(SOCKSRV_OBJ_DIR)/server/stream_channel.o: OP_CXXFLAGS += -Wno-unused-private-field

SOCKSRV_VERSIONED_FILES := server/init.cpp
SOCKSRV_UNVERSIONED_OBJECTS =\
	$(call op_generate_objects,$(filter-out $(SOCKSRV_VERSIONED_FILES),$(SOCKSRV_SOURCES)),$(SOCKSRV_OBJ_DIR))

$(SOCKSRV_OBJ_DIR)/server/init.o: $(SOCKSRV_UNVERSIONED_OBJECTS)
$(SOCKSRV_OBJ_DIR)/server/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

SOCKCLI_SOURCES += \
	client/api_client.cpp \
	client/io_channel_wrapper.cpp \
	client/dgram_channel.cpp \
	client/stream_channel.cpp

SOCKCLI_DEPENDS += socket_common

# See comment above.
$(SOCKCLI_OBJ_DIR)/client/dgram_channel.o: OP_CXXFLAGS += -Wno-unused-private-field
$(SOCKCLI_OBJ_DIR)/client/stream_channel.o: OP_CXXFLAGS += -Wno-unused-private-field

SOCKTEST_SOURCES += \
	$(SOCK_COMMON_SOURCES)

SOCKTEST_DEPENDS += expected
SOCKTEST_LDLIBS += -lrt
