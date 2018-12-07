SOCK_COMMON += \
	common.cpp \
	shared_segment.cpp \
	unix_socket.cpp \
	uuid.cpp

SOCKSRV_SOURCES += \
	$(SOCK_COMMON) \
	eventfd_wrapper.cpp \
	server_api_handler.cpp \
	server_socket.cpp \
	server.cpp \

SOCKSRV_LDLIBS += -lrt

SOCKCLI_SOURCES += \
	$(SOCK_COMMON) \
	client.cpp

SOCKCLI_DEPENDS += expected

SOCKCLI_LDLIBS += -lrt
