#
# Makefile component for models
#

API_DEPENDS += json

MODEL_SOURCES := \
	BulkCreateInterfacesRequest.cpp \
	BulkCreateInterfacesResponse.cpp \
	BulkDeleteInterfacesRequest.cpp \
	Interface_config.cpp \
	Interface.cpp \
	InterfaceProtocolConfig.cpp \
	InterfaceProtocolConfig_eth.cpp \
	InterfaceProtocolConfig_ipv4.cpp \
	InterfaceProtocolConfig_ipv4_dhcp.cpp \
	InterfaceProtocolConfig_ipv4_static.cpp \
	InterfaceStats.cpp \
	ModelBase.cpp \
	PortConfig_bond.cpp \
	PortConfig.cpp \
	PortConfig_dpdk.cpp \
	Port.cpp \
	PortDriver_bond.cpp \
	PortDriver.cpp \
	PortDriver_dpdk.cpp \
	PortDriver_host.cpp \
	PortStatus.cpp \
	Stack.cpp \
	StackElementStats.cpp \
	StackMemoryStats.cpp \
	StackProtocolStats.cpp \
	StackStats.cpp

API_SOURCES += $(addprefix v1/model/,$(MODEL_SOURCES))
API_INCLUDES += v1/model
