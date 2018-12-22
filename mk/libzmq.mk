#
# Makefile component for ZeroMQ
#

LIBZMQ_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(LIBZMQ_REQ_VARS))

LIBZMQ_SRC_DIR := $(ICP_ROOT)/deps/libzmq
LIBZMQ_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/libzmq
LIBZMQ_BLD_DIR := $(ICP_BUILD_ROOT)/libzmq
LIBZMQ_INC_DIR := $(LIBZMQ_BLD_DIR)/include
LIBZMQ_LIB_DIR := $(LIBZMQ_BLD_DIR)/lib

# Update global variables
ICP_INC_DIRS += $(LIBZMQ_INC_DIR)
ICP_LIB_DIRS += $(LIBZMQ_LIB_DIR)
ICP_LDLIBS += -lzmq

###
# Build rules
###
$(LIBZMQ_SRC_DIR)/configure:
	cd $(LIBZMQ_SRC_DIR) && ./autogen.sh

$(LIBZMQ_OBJ_DIR)/Makefile: $(LIBZMQ_SRC_DIR)/configure
	@mkdir -p $(LIBZMQ_OBJ_DIR)
	cd $(LIBZMQ_OBJ_DIR) && $(LIBZMQ_SRC_DIR)/configure $(ICP_CONFIG_OPTS) \
		--enable-drafts=no --prefix="$(LIBZMQ_BLD_DIR)" \
		CC="$(CC)" CFLAGS="$(ICP_COPTS)" \
		CXX="$(CXX)" CXXFLAGS="$(ICP_CXXOPTS)"

$(LIBZMQ_LIB_DIR)/libzmq.la: $(LIBZMQ_OBJ_DIR)/Makefile
	cd $(LIBZMQ_OBJ_DIR) && $(MAKE) install

.PHONY: libzmq
libzmq: $(LIBZMQ_LIB_DIR)/libzmq.la

.PHONY: libzmq_clean
libzmq_clean:
	@rm -rf $(LIBZMQ_OBJ_DIR) $(LIBZMQ_BLD_DIR)
clean: libzmq_clean
