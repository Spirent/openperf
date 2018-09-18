#
# Makefile component for API code
#

API_DEPENDS += pistache

API_INCLUDES += packetio

API_SOURCES += \
	api_init.cpp \
	api_register.c \
	api_route.c \
	packetio/port_routes.cpp

include $(API_SRC_DIR)/v1/model/module.mk
