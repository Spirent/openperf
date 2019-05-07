#
# Makefile component for Pistache C++ HTTP library
#

HTTP_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(HTTP_REQ_VARS))

HTTP_SRC_DIR := $(ICP_ROOT)/deps/pistache
HTTP_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/pistache
HTTP_BLD_DIR := $(ICP_BUILD_ROOT)/pistache
HTTP_INC_DIR := $(HTTP_SRC_DIR)/include
HTTP_LIB_DIR := $(HTTP_BLD_DIR)/lib

# Update global variables
ICP_INC_DIRS += $(HTTP_INC_DIR)
ICP_LIB_DIRS += $(HTTP_LIB_DIR)
ICP_LDLIBS += -lpistache

HTTP_CPPFLAGS := $(addprefix -I,$(HTTP_INC_DIR))
# Pistache code doesn't build without warnings; just silence them since we're
# not going to fix them.
HTTP_CXXFLAGS := -Wno-shadow -Wno-missing-field-initializers $(ICP_CXXSTD)

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
HTTP_OBJECTS := $(patsubst %, $(HTTP_OBJ_DIR)/%, \
	$(patsubst %.cc, %.o, $(HTTP_SOURCES)))

# Pull in object dependencies, maybe
-include $(HTTP_OBJECTS:.o=.d)

HTTP_TARGET := $(HTTP_LIB_DIR)/libpistache.a

###
# Build rules
###
$(HTTP_OBJ_DIR)/%.o: $(HTTP_SRC_DIR)/%.cc
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(HTTP_CPPFLAGS) $(HTTP_CXXFLAGS) $(ICP_COPTS) $<)

$(HTTP_TARGET): $(HTTP_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(HTTP_OBJECTS))

.PHONY: pistache
pistache: $(HTTP_TARGET)

.PHONY: clean_pistache
clean_pistache:
	@rm -rf $(HTTP_OBJ_DIR) $(HTTP_BLD_DIR)
clean: clean_pistache
