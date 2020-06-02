#
# Makefile component for packetio testing
#

PIOTEST_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(PIOTEST_REQ_VARS))

# These packetio defines are required for unit test builds too
OP_PACKETIO_DRIVER := dpdk
PIO_SRC_DIR := $(OP_ROOT)/src/modules/packetio

PIOTEST_SRC_DIR := $(OP_ROOT)/src/modules/packetio
PIOTEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packetio
PIOTEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

PIOTEST_SOURCES :=
PIOTEST_DEPENDS := immer
PIOTEST_LDLIBS :=

OP_INC_DIRS += $(OP_ROOT)/src/modules

# Add unit tests directory for utility headers
OP_INC_DIRS += $(OP_ROOT)/tests/unit/modules

include $(PIOTEST_SRC_DIR)/directory.mk

PIOTEST_OBJECTS := $(call op_generate_objects,$(PIOTEST_SOURCES),$(PIOTEST_OBJ_DIR))

PIOTEST_LIBRARY := openperf_packetio_test
PIOTEST_TARGET := $(PIOTEST_LIB_DIR)/lib$(PIOTEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(PIOTEST_LIBRARY) -Wl,--no-whole-archive $(PIOTEST_LDLIBS)

# Load external dependencies
-include $(PIOTEST_OBJECTS:.o=.d)

$(call op_include_dependencies,$(PIOTEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PIOTEST_SOURCES),PIOTEST_SRC_DIR,PIOTEST_OBJ_DIR,PIOTEST_DEPENDS))
$(eval $(call op_generate_clean_rules,packetio_test,PIOTEST_TARGET,PIOTEST_OBJECTS))

$(PIOTEST_TARGET): $(PIOTEST_OBJECTS)
	$(call op_link_library,$@,$(PIOTEST_OBJECTS))

.PHONY: packetio_test
packetio_test: $(PIOTEST_TARGET)
