SOCK_COMMON += \
	common.cpp \
	shared_segment.cpp \
	unix_socket.cpp \
	uuid.cpp

SOCKSRV_SOURCES += \
	$(SOCK_COMMON) \
	server/api_handler.cpp \
	server/io_channel.cpp \
	server/socket.cpp \
	server/socket_utils.cpp \
	server/api_server.cpp \
	server/tcp_socket.cpp \
	server/udp_socket.cpp

SOCKSRV_LDLIBS += -lrt

SOCKCLI_SOURCES += \
	$(SOCK_COMMON) \
	client/api_client.cpp \
	client/io_channel.cpp

SOCKCLI_DEPENDS += expected

SOCKCLI_LDLIBS += -lrt
