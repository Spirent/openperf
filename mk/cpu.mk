#
# Makefile component for CPU generator code
#

CPU_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT \
	OP_ISPC \
	OP_ISPC_TARGETS
$(call op_check_vars,$(CPU_REQ_VARS))

CPU_SRC_DIR := $(OP_ROOT)/src/modules/cpu
CPU_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/cpu
CPU_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(CPU_LIB_DIR)
CPU_ISPC_FLAGS := $(OP_ISPC_FLAGS)

CPU_SOURCES :=
CPU_DEPENDS :=
CPU_LDLIBS :=

include $(CPU_SRC_DIR)/directory.mk

CPU_OBJECTS := $(call op_generate_objects,$(filter-out %.ispc,$(CPU_SOURCES)),$(CPU_OBJ_DIR))

###
# ISPC generates multiple object files when using multiple targets.
# Translate targets and options into proper object names
###
CPU_ISPC_HEADERS := $(patsubst %, $(CPU_OBJ_DIR)/%, \
	$(notdir $(patsubst %.ispc, %.ispc.h, $(filter %.ispc, $(CPU_SOURCES)))))

CPU_ISPC_OBJECTS := $(patsubst %, $(CPU_OBJ_DIR)/%, \
	$(patsubst %.ispc, %.o, $(filter %.ispc, $(CPU_SOURCES))))

# Adjust objects based on the number of ISPC target architectures
CPU_DEFINES :=
CPU_ISPC_TARGET_OBJECTS :=

ifeq (,$(word 2,$(OP_ISPC_TARGETS)))  # i.e. there is no 2nd target
	CPU_DEFINES += ISPC_TARGET_AUTOMATIC
	CPU_ISPC_TARGET_OBJECTS += $(CPU_ISPC_OBJECTS)
else
	ifneq (,$(filter sse2-%,$(OP_ISPC_TARGETS)))
		CPU_DEFINES += ISPC_TARGET_SSE2
		CPU_ISPC_TARGET_OBJECTS += $(addsuffix _sse2.o,$(basename $(CPU_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter sse4-%,$(OP_ISPC_TARGETS)))
		CPU_DEFINES += ISPC_TARGET_SSE4
		CPU_ISPC_TARGET_OBJECTS += $(addsuffix _sse4.o,$(basename $(CPU_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx1-%,$(OP_ISPC_TARGETS)))
		CPU_DEFINES += ISPC_TARGET_AVX
		CPU_ISPC_TARGET_OBJECTS += $(addsuffix _avx.o,$(basename $(CPU_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx2-%,$(OP_ISPC_TARGETS)))
		CPU_DEFINES += ISPC_TARGET_AVX2
		CPU_ISPC_TARGET_OBJECTS += $(addsuffix _avx2.o,$(basename $(CPU_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx512skx-%,$(OP_ISPC_TARGETS)))
		CPU_DEFINES += ISPC_TARGET_AVX512SKX
		CPU_ISPC_TARGET_OBJECTS += $(addsuffix _avx512skx.o,$(basename $(CPU_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter neon-%,$(OP_ISPC_TARGETS)))
		CPU_DEFINES += ISPC_TARGET_NEON
		CPU_ISPC_TARGET_OBJECTS += $(addsuffix _neon.o,$(basename $(CPU_ISPC_OBJECTS)))
	endif

# The ISPC target objects depend on the source "objects"
$(CPU_ISPC_TARGET_OBJECTS): $(CPU_ISPC_OBJECTS)
endif

CPU_FLAGS := $(addprefix -D,$(CPU_DEFINES))
CPU_LIBRARY := openperf_cpu
CPU_TARGET := $(CPU_LIB_DIR)/lib$(CPU_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(CPU_LIBRARY) -Wl,--no-whole-archive $(CPU_LDLIBS)

-include $(CPU_OBJECTS:.o=.d)
-include $(CPU_ISPC_OBJECTS:.o=.d)

# Load external dependencies
$(call op_include_dependencies,$(CPU_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(CPU_SOURCES),CPU_SRC_DIR,CPU_OBJ_DIR,CPU_DEPENDS,CPU_FLAGS))
$(eval $(call op_generate_clean_rules,cpu,CPU_TARGET,CPU_OBJECTS))

# In addition to the standard rules above, we need a couple of ISPC rules
# Note: that the sed command on the bottom is because ispc doesn't write
# a target name into the dependency when creating multiple targets.  So,
# we write the target name in ourselves.
CPU_COMMA := ,
CPU_EMPTY :=
CPU_SPACE := $(CPU_EMPTY) $(CPU_EMPTY)
CPU_ISPC_FLAGS := --werror $(OP_ISPC_FLAGS)

ifeq ($(MODE),release)
	CPU_ISPC_FLAGS += --opt=disable-assertions
else
	CPU_ISPC_FLAGS += -g
	ifeq ($(MODE),debug)
		CPU_ISPC_FLAGS += -O0
	endif
endif

$(CPU_OBJ_DIR)/%.o: $(CPU_SRC_DIR)/%.ispc
	@mkdir -p $(dir $@)
	$(strip $(OP_ISPC) -o $@ $(CPU_ISPC_FLAGS) \
		-M -MF $(@:.o=.d) \
		--target=$(subst $(CPU_SPACE),$(CPU_COMMA),$(OP_ISPC_TARGETS)) \
		--header-outfile=$(patsubst %,$(CPU_OBJ_DIR)/%,$(notdir $<.h)) $<)
	@sed -i -e s,\(null\),$@, $(@:.o=.d)

clean_cpu: clean_cpu_ispc
clean_cpu_ispc:
	@rm -f $(CPU_ISPC_OBJECTS) $(CPU_ISPC_TARGET_OBJECTS)

$(CPU_TARGET): $(CPU_OBJECTS) $(CPU_ISPC_TARGET_OBJECTS)
	$(call op_link_library,$@,$(CPU_OBJECTS) $(CPU_ISPC_TARGET_OBJECTS))

.PHONY: cpu
cpu: $(CPU_TARGET)
