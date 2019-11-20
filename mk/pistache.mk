#
# Makefile component for Pistache C++ HTTP library
#

HTTP_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(HTTP_REQ_VARS))

HTTP_SRC_DIR := $(OP_ROOT)/deps/pistache
HTTP_OBJ_DIR := $(OP_BUILD_ROOT)/obj/pistache
HTTP_BLD_DIR := $(OP_BUILD_ROOT)/pistache
HTTP_INC_DIR := $(HTTP_SRC_DIR)/include
HTTP_LIB_DIR := $(HTTP_BLD_DIR)/lib

# Update global variables
OP_INC_DIRS += $(HTTP_INC_DIR)
OP_LIB_DIRS += $(HTTP_LIB_DIR)
OP_LDLIBS += -lpistache

# Pistache code doesn't build without warnings; just silence them since we're
# not going to fix them.
HTTP_FLAGS := -Wno-shadow -Wno-missing-field-initializers

###
# Sources, objects, targets, and dependencies
###
HTTP_COMMON_SOURCES := \
	common/cookie.cc \
	common/description.cc \
	common/http.cc \
	common/http_defs.cc \
	common/http_header.cc \
	common/http_headers.cc \
	common/mime.cc \
	common/net.cc \
	common/os.cc \
	common/peer.cc \
	common/reactor.cc \
	common/stream.cc \
	common/tcp.cc \
	common/timer_pool.cc \
	common/transport.cc

HTTP_CLIENT_SOURCES := \
	client/client.cc

HTTP_SERVER_SOURCES := \
	server/endpoint.cc \
	server/listener.cc \
	server/router.cc

HTTP_SOURCES := $(addprefix src/,$(HTTP_COMMON_SOURCES) $(HTTP_CLIENT_SOURCES) $(HTTP_SERVER_SOURCES))
HTTP_OBJECTS := $(call op_generate_objects,$(HTTP_SOURCES),$(HTTP_OBJ_DIR))

# Pull in object dependencies, maybe
-include $(HTTP_OBJECTS:.o=.d)

HTTP_TARGET := $(HTTP_LIB_DIR)/libpistache.a

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(HTTP_SOURCES),HTTP_SRC_DIR,HTTP_OBJ_DIR,,HTTP_FLAGS))
$(eval $(call op_generate_clean_rules,pistache,HTTP_TARGET,HTTP_OBJECTS))

$(HTTP_TARGET): $(HTTP_OBJECTS)
	$(call op_link_library,$@,$(HTTP_OBJECTS))

.PHONY: pistache
pistache: $(HTTP_TARGET)
