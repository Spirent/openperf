#
# Makefile component for packet IO infrastructure
#

PIO_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT \
	OP_PACKETIO_DRIVER
$(call op_check_vars,$(PIO_REQ_VARS))

PIO_SOURCES :=
PIO_INCLUDES :=
PIO_DEPENDS :=
PIO_LIBS :=

PIO_SRC_DIR := $(OP_ROOT)/src/modules/packetio
PIO_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packetio
PIO_LIB_DIR := $(OP_BUILD_ROOT)/lib

include $(PIO_SRC_DIR)/directory.mk

PIO_OBJECTS := $(call op_generate_objects,$(PIO_SOURCES),$(PIO_OBJ_DIR))

PIO_INC_DIRS := $(dir $(PIO_SRC_DIR)) $(addprefix $(PIO_SRC_DIR)/,$(PIO_INCLUDES))
PIO_FLAGS := $(addprefix -I,$(PIO_INC_DIRS))

PIO_LIBRARY := openperf_packetio-$(OP_PACKETIO_DRIVER)
PIO_TARGET := $(PIO_LIB_DIR)/lib$(PIO_LIBRARY).a

OP_INC_DIRS += $(PIO_INC_DIRS)
OP_LIB_DIRS += $(PIO_LIB_DIR)
OP_LDLIBS += -Wl,--whole-archive -l$(PIO_LIBRARY) -Wl,--no-whole-archive $(PIO_LIBS)

# Load packet-io dependencies
-include $(PIO_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PIO_DEPENDS))

###
# DPDK Hack.
# This should really come from the dpdk.mk include, but that will require some
# clever makefile hackery... Just do this for now.
# DPDK users the keyword register, which isn't c++17 compliant
###
PIO_FLAGS += -Wno-register

###
# We link the lwip compilation objects directly into the packetio target.  At the
# moment, packetio can't really function without lwIP and the lwIP configuration
# file is part of the packetio module, so the two are *very* tightly coupled.
###
include $(OP_ROOT)/mk/lwip.mk

LWIP_REQ_VARS := \
	LWIP_OBJECTS
$(call op_check_vars,$(LWIP_REQ_VARS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PIO_SOURCES),PIO_SRC_DIR,PIO_OBJ_DIR,PIO_DEPENDS,PIO_FLAGS))
$(eval $(call op_generate_clean_rules,packetio,PIO_TARGET,PIO_OBJECTS))

$(PIO_TARGET): $(PIO_OBJECTS) $(LWIP_OBJECTS)
	$(call op_link_library,$@,$(PIO_OBJECTS) $(LWIP_OBJECTS))

.PHONY: packetio
packetio: $(PIO_TARGET)

clean_packetio: clean_lwip
