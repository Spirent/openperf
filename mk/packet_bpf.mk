#
# Makefile component for BPF code
#

BPF_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(BPF_REQ_VARS))

BPF_SRC_DIR := $(OP_ROOT)/src/modules/packet/bpf
BPF_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/bpf
BPF_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(BPF_LIB_DIR)

BPF_SOURCES :=
BPF_DEPENDS :=
BPF_LDLIBS :=

include $(BPF_SRC_DIR)/directory.mk

BPF_OBJECTS := $(call op_generate_objects,$(BPF_SOURCES),$(BPF_OBJ_DIR))

BPF_LIBRARY := openperf_packet_bpf
BPF_TARGET := $(BPF_LIB_DIR)/lib$(BPF_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(BPF_LIBRARY) -Wl,--no-whole-archive $(BPF_LDLIBS)

# Load external dependencies
-include $(BPF_OBJECTS:.o=.d)
$(call op_include_dependencies,$(BPF_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(BPF_SOURCES),BPF_SRC_DIR,BPF_OBJ_DIR,BPF_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_bpf,BPF_TARGET,BPF_OBJECTS))

$(BPF_TARGET): $(BPF_OBJECTS)
	$(call op_link_library,$@,$(BPF_OBJECTS))

.PHONY: packet_bpf
packet_bpf: $(BPF_TARGET)
