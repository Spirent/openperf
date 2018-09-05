#
# Makefile component for cJSON
#

CWEB_SRC_DIR := $(ICP_ROOT)/deps/civetweb
CWEB_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/civetweb
CWEB_BLD_DIR := $(ICP_BUILD_ROOT)/civetweb
CWEB_INC_DIR := $(CWEB_BLD_DIR)/include
CWEB_LIB_DIR := $(CWEB_BLD_DIR)/lib

# Update global variables
ICP_INC_DIRS += $(CWEB_INC_DIR)
ICP_LIB_DIRS += $(CWEB_LIB_DIR)
ICP_LDLIBS += -lcivetweb

###
# Build rules
###

CWEB_CFLAGS := -Wall -Wextra -Wshadow -Wformat-security -Winit-self \
	-Wmissing-prototypes -fstack-protector-strong

# Don't generate warnings for silly GCC pragmas
CWEB_CFLAGS += -Wno-unknown-warning-option

CWEB_DEFINES := \
	NO_FILES \
	NO_SSL \
	NO_CGI \
	USE_IPV6 \
	USE_WEBSOCKET \
	NO_CACHING \
	USE_STACK_SIZE=102400

ifeq ($(PLATFORM),"linux")
	CWEB_DEFINES += LINUX
endif

CWEB_DEFINES := $(addprefix -D,$(CWEB_DEFINES))
ICP_DEFINES += $(CWEB_DEFINES)

$(CWEB_OBJ_DIR)/civetweb.o: $(CWEB_SRC_DIR)/src/civetweb.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c -I$(CWEB_SRC_DIR)/include $(CWEB_CFLAGS) \
		$(ICP_COPTS) $(CWEB_DEFINES) $<)

$(CWEB_LIB_DIR)/libcivetweb.a: $(CWEB_OBJ_DIR)/civetweb.o
	@mkdir -p $(dir $@)
	$(AR) cq $@ $<

$(CWEB_INC_DIR)/civetweb.h: $(CWEB_SRC_DIR)/include/civetweb.h
	@mkdir -p $(dir $@)
	@cp $< $@

.PHONY: civetweb
civetweb: $(CWEB_LIB_DIR)/libcivetweb.a $(CWEB_INC_DIR)/civetweb.h

.PHONY: clean_civetweb
clean_civetweb:
	@rm -rf $(CWEB_OBJ_DIR) $(CWEB_BLD_DIR)
clean: clean_civetweb
