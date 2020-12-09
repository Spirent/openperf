#
# Makefile component for LwIP based stack module
#

$(call op_check_vars, OP_PACKETIO_DRIVER)

STACK_DEPENDS += framework packetio
STACK_INCLUDES += lwip/include

STACK_SOURCES += \
	api_strings.cpp \
	api_transmogrify.cpp \
	init.cpp \
	handler.cpp \
	lwip/memp.c \
	lwip/ipv6_netifapi.c \
	lwip/netif_wrapper.cpp \
	lwip/tcp_in.c \
	lwip/tcpip.cpp \
	lwip/tcp_out.c \
	lwip/tcp_stubs.c \
	server.cpp \
	utils.cpp

# packet sockets require linux headers
ifeq ($(PLATFORM), linux)
	STACK_SOURCES += lwip/packet.cpp
endif

include $(STACK_SRC_DIR)/$(OP_PACKETIO_DRIVER)/directory.mk

STACK_VERSIONED_FILES := init.cpp
STACK_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(STACK_VERSIONED_FILES),$(STACK_SOURCES)),$(STACK_OBJ_DIR))

$(STACK_OBJ_DIR)/init.o : $(STACK_UNVERSIONED_OBJECTS)
$(STACK_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
