#
# Makefile component for libpcap
#

LIBPCAP_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(LIBPCAP_REQ_VARS))

LIBPCAP_SRC_DIR := $(OP_ROOT)/deps/libpcap
LIBPCAP_OBJ_DIR := $(OP_BUILD_ROOT)/obj/libpcap
LIBPCAP_BLD_DIR := $(OP_BUILD_ROOT)/libpcap
LIBPCAP_INC_DIR := $(LIBPCAP_BLD_DIR)/include
LIBPCAP_LIB_DIR := $(LIBPCAP_BLD_DIR)/lib

# Update global variables
OP_INC_DIRS += $(LIBPCAP_INC_DIR)
OP_LIB_DIRS += $(LIBPCAP_LIB_DIR)
OP_LDLIBS += -lpcap

LIBPCAP_CONFIG_OPTS += \
	--enable-usb=no \
	--enable-netmap=no \
	--enable-bluetooth=no \
	--enable-dbus=no \
	--enable-rdma=no \
	--enable-shared=no \
	--without-libnl

###
# Build rules
###
$(LIBPCAP_SRC_DIR)/configure:
	cd $(LIBPCAP_SRC_DIR) && ./autogen.sh

$(LIBPCAP_OBJ_DIR)/Makefile: $(LIBPCAP_SRC_DIR)/configure
	@mkdir -p $(LIBPCAP_OBJ_DIR)
	cd $(LIBPCAP_OBJ_DIR) && $(LIBPCAP_SRC_DIR)/configure $(OP_CONFIG_OPTS) \
		$(LIBPCAP_CONFIG_OPTS) --prefix="$(LIBPCAP_BLD_DIR)" \
		CC="$(CC)" CFLAGS="$(OP_COPTS)" \
		CXX="$(CXX)" CXXFLAGS="$(OP_CXXOPTS)"

# DPDK and libpcap both have a bpf_validate() function
# Since this function appears to be included for compatibility reasons
# and openperf doesn't need it, strip it to prevent conflicts with DPDK
LIBPCAP_STRIP_CONFLICTING_SYMBOLS := \
	objcopy --strip-symbol bpf_validate $(LIBPCAP_LIB_DIR)/libpcap.a

$(LIBPCAP_LIB_DIR)/libpcap.a: $(LIBPCAP_OBJ_DIR)/Makefile
	cd $(LIBPCAP_OBJ_DIR) && $(MAKE) install && \
	$(LIBPCAP_STRIP_CONFLICTING_SYMBOLS)

.PHONY: libpcap
libpcap: $(LIBPCAP_LIB_DIR)/libpcap.a

.PHONY: libpcap_clean
libpcap_clean:
	@rm -rf $(LIBPCAP_OBJ_DIR) $(LIBPCAP_BLD_DIR)
clean: libpcap_clean
