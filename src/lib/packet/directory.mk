PKT_SOURCES += \
	type/ipv4_address.cpp \
	type/ipv4_network.cpp \
	type/ipv6_address.cpp \
	type/ipv6_network.cpp \
	type/mac_address.cpp

# Pull in all protocol files and format them for our build rules
PKT_SOURCES += $(patsubst $(PKT_SRC_DIR)/%,%,$(wildcard $(PKT_SRC_DIR)/protocol/*.cpp))
