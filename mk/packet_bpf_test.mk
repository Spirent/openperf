#
# Makefile component for packet BPF test code
#

BPF_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(BPF_TEST_REQ_VARS))

BPF_SRC_DIR := $(OP_ROOT)/src/modules/packet/bpf

BPF_TEST_SRC_DIR := $(OP_ROOT)/src/modules/packet/bpf
BPF_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/bpf
BPF_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(BPF_TEST_LIB_DIR)

BPF_TEST_SOURCES :=
BPF_TEST_DEPENDS :=
BPF_TEST_LDLIBS :=

include $(BPF_TEST_SRC_DIR)/directory.mk

BPF_TEST_OBJECTS := $(call op_generate_objects,$(BPF_TEST_SOURCES),$(BPF_TEST_OBJ_DIR))

BPF_TEST_LIBRARY := openperf_packet_bpf_test
BPF_TEST_TARGET := $(BPF_TEST_LIB_DIR)/lib$(BPF_TEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(BPF_TEST_LIBRARY) -Wl,--no-whole-archive $(BPF_TEST_LDLIBS)

# Load external dependencies
-include $(BPF_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(BPF_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(BPF_TEST_SOURCES),BPF_TEST_SRC_DIR,BPF_TEST_OBJ_DIR,BPF_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_bpf_test,BPF_TEST_TARGET,BPF_TEST_OBJECTS))

$(BPF_TEST_TARGET): $(BPF_TEST_OBJECTS)
	$(call op_link_library,$@,$(BPF_TEST_OBJECTS))

.PHONY: packet_bpf_test
packet_bpf_test: $(BPF_TEST_TARGET)
