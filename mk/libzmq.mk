#
# Makefile component for ZeroMQ
#

LIBZMQ_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(LIBZMQ_REQ_VARS))

LIBZMQ_SRC_DIR := $(OP_ROOT)/deps/libzmq
LIBZMQ_OBJ_DIR := $(OP_BUILD_ROOT)/obj/libzmq
LIBZMQ_BLD_DIR := $(OP_BUILD_ROOT)/libzmq
LIBZMQ_INC_DIR := $(LIBZMQ_SRC_DIR)/include
LIBZMQ_LIB_DIR := $(LIBZMQ_BLD_DIR)/lib

# Update global variables
OP_INC_DIRS += $(LIBZMQ_INC_DIR)
OP_LIB_DIRS += $(LIBZMQ_LIB_DIR)
OP_LDLIBS += -lzmq

###
# Build rules
###
$(LIBZMQ_SRC_DIR)/configure:
	cd $(LIBZMQ_SRC_DIR) && ./autogen.sh

LIBZMQ_CONFIG_OPTS := --with-libgssapi_krb5=no
ifeq ($(MODE),debug)
	LIBZMQ_CONFIG_OPTS += --enable-debug
endif

$(LIBZMQ_OBJ_DIR)/Makefile: $(LIBZMQ_SRC_DIR)/configure
	@mkdir -p $(LIBZMQ_OBJ_DIR)
	cd $(LIBZMQ_OBJ_DIR) && $(LIBZMQ_SRC_DIR)/configure $(OP_CONFIG_OPTS) \
		$(LIBZMQ_CONFIG_OPTS) --enable-drafts=no --prefix="$(LIBZMQ_BLD_DIR)" \
		CC="$(CC)" CFLAGS="$(OP_COPTS) $(OP_CROSSFLAGS) $(OP_EXTRA_CFLAGS)" \
		CXX="$(CXX)" CXXFLAGS="$(OP_COPTS) $(OP_CROSSFLAGS) $(OP_EXTRA_CXXFLAGS)" \
		CPPFLAGS="$(OP_EXTRA_CPPFLAGS)" \
		LDFLAGS="$(OP_LDOPTS) $(OP_CROSSFLAGS) $(OP_EXTRA_LDFLAGS)"

$(LIBZMQ_LIB_DIR)/libzmq.la: $(LIBZMQ_OBJ_DIR)/Makefile
	cd $(LIBZMQ_OBJ_DIR) && $(MAKE) install

.PHONY: libzmq
libzmq: $(LIBZMQ_LIB_DIR)/libzmq.la

.PHONY: clean_libzmq
clean_libzmq:
	@rm -rf $(LIBZMQ_OBJ_DIR) $(LIBZMQ_BLD_DIR) $(LIBZMQ_SRC_DIR)/configure
clean: clean_libzmq
