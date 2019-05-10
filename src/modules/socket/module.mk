SOCK_COMMON += \
	common.cpp \
	pbuf_vec.cpp \
	shared_segment.cpp \
	unix_socket.cpp \
	uuid.cpp

SOCKSRV_SOURCES += \
	$(SOCK_COMMON) \
	server/api_handler.cpp \
	server/api_server_options.c \
	server/dgram_channel.cpp \
	server/lwip_tcp_event.cpp \
	server/lwip_utils.cpp \
	server/pbuf_queue.cpp \
	server/stream_channel.cpp \
	server/socket_utils.cpp \
	server/api_server.cpp \
	server/tcp_socket.cpp \
	server/udp_socket.cpp

SOCKSRV_LDLIBS += -lrt

# Each channel object lives in shared memory and has both a client and server
# "version".  Each version contains the same members, but client's don't touch
# server data and vice versa.  Hence, this warning is safe to ignore.
$(SOCKSRV_OBJ_DIR)/server/dgram_channel.o: ICP_CXXFLAGS += -Wno-unused-private-field
$(SOCKSRV_OBJ_DIR)/server/stream_channel.o: ICP_CXXFLAGS += -Wno-unused-private-field

SOCKCLI_SOURCES += \
	$(SOCK_COMMON) \
	client/api_client.cpp \
	client/io_channel_wrapper.cpp \
	client/dgram_channel.cpp \
	client/stream_channel.cpp

SOCKCLI_DEPENDS += expected
SOCKCLI_LDLIBS += -lrt

# See comment above.
$(SOCKCLI_OBJ_DIR)/client/dgram_channel.o: ICP_CXXFLAGS += -Wno-unused-private-field
$(SOCKCLI_OBJ_DIR)/client/stream_channel.o: ICP_CXXFLAGS += -Wno-unused-private-field

SOCKTEST_SOURCES += \
	$(SOCK_COMMON)

SOCKTEST_DEPENDS += expected
SOCKTEST_LDLIBS += -lrt
