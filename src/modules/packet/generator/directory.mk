#
# Makefile component to build Packet Generator code
#

PG_DEPENDS += api base_n framework versions packet_protocol range_v3

PG_SOURCES += \
	api_strings.cpp \
	api_transmogrify.cpp \
	handler.cpp \
	init.cpp \
	learning.cpp \
	server.cpp \
	source.cpp \
	source_transmogrify.cpp \
	traffic_transmogrify.cpp \
	traffic/length_template.cpp \
	traffic/header/explode.cpp \
	traffic/header/utils.cpp \
	traffic/header/expand_impl/custom.cpp \
	traffic/header/expand_impl/ethernet.cpp \
	traffic/header/expand_impl/mpls.cpp \
	traffic/header/expand_impl/vlan.cpp \
	traffic/header/expand_impl/ipv4.cpp \
	traffic/header/expand_impl/ipv6.cpp \
	traffic/header/expand_impl/tcp.cpp \
	traffic/header/expand_impl/udp.cpp \
	traffic/packet_template.cpp \
	traffic/protocol/custom.cpp \
	traffic/protocol/ethernet.cpp \
	traffic/protocol/ip.cpp \
	traffic/protocol/mpls.cpp \
	traffic/protocol/protocol.cpp \
	traffic/protocol/vlan.cpp \
	traffic/sequence.cpp \
	validation.cpp

PG_VERSIONED_FILES := init.cpp
PG_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(PG_VERSIONED_FILES),$(PG_SOURCES)),$(PG_OBJ_DIR))

$(PG_OBJ_DIR)/init.o: $(PG_UNVERSIONED_OBJECTS)
$(PG_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""

PG_TEST_DEPENDS += expected framework json range_v3 packet_protocol

PG_TEST_SOURCES += \
	traffic/length_template.cpp \
	traffic/header/explode.cpp \
	traffic/header/utils.cpp \
	traffic/header/expand_impl/custom.cpp \
	traffic/header/expand_impl/ethernet.cpp \
	traffic/header/expand_impl/mpls.cpp \
	traffic/header/expand_impl/vlan.cpp \
	traffic/header/expand_impl/ipv4.cpp \
	traffic/header/expand_impl/ipv6.cpp \
	traffic/header/expand_impl/tcp.cpp \
	traffic/header/expand_impl/udp.cpp \
	traffic/packet_template.cpp \
	traffic/protocol/custom.cpp \
	traffic/protocol/ethernet.cpp \
	traffic/protocol/ip.cpp \
	traffic/protocol/mpls.cpp \
	traffic/protocol/protocol.cpp \
	traffic/protocol/vlan.cpp \
	traffic/sequence.cpp
