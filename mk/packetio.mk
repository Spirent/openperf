#
# Makefile component for packet IO infrastructure
#

PIO_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT \
	ICP_PACKETIO_DRIVER
$(call icp_check_vars,$(PIO_REQ_VARS))

PIO_SOURCES :=
PIO_INCLUDES :=
PIO_DEPENDS :=
PIO_LIBS :=

PIO_SRC_DIR := $(ICP_ROOT)/src/modules/packetio
PIO_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/modules/packetio
PIO_LIB_DIR := $(ICP_BUILD_ROOT)/lib

include $(PIO_SRC_DIR)/module.mk

PIO_OBJECTS := $(patsubst %, $(PIO_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(PIO_SOURCES))))

PIO_OBJECTS += $(patsubst %, $(PIO_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(PIO_SOURCES))))

# Pull in object dependencies, maybe
-include $(PIO_OBJECTS:.o=.d)

PIO_INC_DIRS := $(dir $(PIO_SRC_DIR)) $(addprefix $(PIO_SRC_DIR)/,$(PIO_INCLUDES))
PIO_CPPFLAGS := $(addprefix -I,$(PIO_INC_DIRS))

PIO_LIBRARY := icp_packetio-$(ICP_PACKETIO_DRIVER)
PIO_TARGET := $(PIO_LIB_DIR)/lib$(PIO_LIBRARY).a

ICP_INC_DIRS += $(PIO_INC_DIRS)
ICP_LIB_DIRS += $(PIO_LIB_DIR)
ICP_LDLIBS += -Wl,--whole-archive -l$(PIO_LIBRARY) -Wl,--no-whole-archive $(PIO_LIBS)

# Load packet-io dependencies
$(foreach DEP,$(PIO_DEPENDS),$(eval include $(ICP_ROOT)/mk/$(DEP).mk))

###
# DPDK Hack.
# This should really come from the dpdk.mk include, but that will require some
# clever makefile hackery... Just do this for now.
# DPDK users the keyword register, which isn't c++17 compliant
###
PIO_CXXFLAGS += -Wno-register

###
# We link the lwip compilation objects directly into the packetio target.  At the
# moment, packetio can't really function without lwIP and the lwIP configuration
# file is part of the packetio module, so the two are *very* tightly coupled.
###

include $(ICP_ROOT)/mk/lwip.mk

LWIP_REQ_VARS := \
	LWIP_OBJECTS
$(call icp_check_vars,$(LWIP_REQ_VARS))

###
# Build rules
###
$(PIO_OBJECTS): | $(PIO_DEPENDS) libzmq

$(PIO_OBJ_DIR)/%.o: $(PIO_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(PIO_CPPFLAGS) $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $<)

$(PIO_OBJ_DIR)/%.o: $(PIO_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(PIO_CPPFLAGS) $(ICP_CPPFLAGS) $(PIO_CXXFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $<)

$(PIO_TARGET): $(PIO_OBJECTS) $(LWIP_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(PIO_OBJECTS) $(LWIP_OBJECTS))

.PHONY: packetio
packetio: $(PIO_TARGET)

.PHONY: clean_packetio
clean_packetio: clean_lwip
	@rm -rf $(PIO_OBJ_DIR) $(PIO_TARGET)
clean: clean_packetio
