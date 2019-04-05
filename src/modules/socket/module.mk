SOCK_COMMON += \
	common.cpp \
	shared_segment.cpp \
	unix_socket.cpp \
	uuid.cpp

SOCKSRV_SOURCES += \
	$(SOCK_COMMON) \
	server/api_handler.cpp \
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

SOCKCLI_SOURCES += \
	$(SOCK_COMMON) \
	client/api_client.cpp \
	client/io_channel_wrapper.cpp \
	client/dgram_channel.cpp \
	client/stream_channel.cpp

SOCKCLI_DEPENDS += expected

SOCKCLI_LDLIBS += -lrt

SOCKTEST_SOURCES += \
	$(SOCK_COMMON)

SOCKTEST_DEPENDS += expected

SOCKTEST_LDLIBS += -lrt
