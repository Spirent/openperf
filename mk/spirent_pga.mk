#
# Makefile component for the Spirent Packet Generation/Analysis library
#

PGA_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT \
	OP_ISPC \
	OP_ISPC_TARGETS
$(call op_check_vars,$(PGA_REQ_VARS))

# Need these to generate a comma separated target list for ISPC
PGA_COMMA := ,
PGA_EMPTY :=
PGA_SPACE := $(PGA_EMPTY) $(PGA_EMPTY)

PGA_ISPC_OPTS :=

ifeq ($(MODE),debug)
	PGA_ISPC_OPTS += -O0
endif
ifneq ($(MODE),release)
	PGA_ISPC_OPTS += -g
endif

PGA_SRC_DIR := $(OP_ROOT)/src/lib/spirent_pga
PGA_INC_DIR := $(dir $(PGA_SRC_DIR))
PGA_OBJ_DIR := $(OP_BUILD_ROOT)/obj/lib/spirent_pga
PGA_LIB_DIR := $(OP_BUILD_ROOT)/lib

PGA_SOURCES :=

include $(PGA_SRC_DIR)/directory.mk

PGA_OBJECTS := $(call op_generate_objects,$(filter-out %.ispc,$(PGA_SOURCES)),$(PGA_OBJ_DIR))

###
# ISPC generates multiple object files when using multiple targets.
# Translate targets and options into proper object names
###
PGA_ISPC_HEADERS := $(patsubst %, $(PGA_OBJ_DIR)/%, \
	$(notdir $(patsubst %.ispc, %.ispc.h, $(filter %.ispc, $(PGA_SOURCES)))))

PGA_ISPC_OBJECTS := $(patsubst %, $(PGA_OBJ_DIR)/%, \
	$(patsubst %.ispc, %.o, $(filter %.ispc, $(PGA_SOURCES))))

# Adjust objects based on the number of ISPC target architectures
PGA_DEFINES :=
PGA_ISPC_TARGET_OBJECTS :=

ifeq (,$(word 2,$(OP_ISPC_TARGETS)))  # i.e. there is no 2nd target
	PGA_DEFINES += ISPC_TARGET_AUTOMATIC
	PGA_ISPC_TARGET_OBJECTS += $(PGA_ISPC_OBJECTS)
else
	ifneq (,$(filter sse2-%,$(OP_ISPC_TARGETS)))
		PGA_DEFINES += ISPC_TARGET_SSE2
		PGA_ISPC_TARGET_OBJECTS += $(addsuffix _sse2.o,$(basename $(PGA_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter sse4-%,$(OP_ISPC_TARGETS)))
		PGA_DEFINES += ISPC_TARGET_SSE4
		PGA_ISPC_TARGET_OBJECTS += $(addsuffix _sse4.o,$(basename $(PGA_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx1-%,$(OP_ISPC_TARGETS)))
		PGA_DEFINES += ISPC_TARGET_AVX
		PGA_ISPC_TARGET_OBJECTS += $(addsuffix _avx.o,$(basename $(PGA_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx2-%,$(OP_ISPC_TARGETS)))
		PGA_DEFINES += ISPC_TARGET_AVX2
		PGA_ISPC_TARGET_OBJECTS += $(addsuffix _avx2.o,$(basename $(PGA_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx512skx-%,$(OP_ISPC_TARGETS)))
		PGA_DEFINES += ISPC_TARGET_AVX512SKX
		PGA_ISPC_TARGET_OBJECTS += $(addsuffix _avx512skx.o,$(basename $(PGA_ISPC_OBJECTS)))
	endif
endif

# Turn defines into proper compiler flags
PGA_FLAGS := -mpclmul $(addprefix -D,$(PGA_DEFINES))

PGA_LIBRARY := openperf_spirent-pga
PGA_TARGET := $(PGA_LIB_DIR)/lib$(PGA_LIBRARY).a

OP_INC_DIRS += $(PGA_INC_DIR)
OP_LIB_DIRS += $(PGA_LIB_DIR)
OP_LDLIBS += -l$(PGA_LIBRARY)

-include $(PGA_OBJECTS:.o=.d)
-include $(PGA_ISPC_OBJECTS:.o=.d)

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PGA_SOURCES),PGA_SRC_DIR,PGA_OBJ_DIR,,PGA_FLAGS))
$(eval $(call op_generate_clean_rules,spirent_pga,PGA_TARGET,PGA_OBJECTS))

# The ISPC target objects depend on their source "objects"
$(PGA_ISPC_TARGET_OBJECTS): $(PGA_ISPC_OBJECTS)

# In addition to the standard rules above, we need a couple of ISPC rules
# Note: that the sed command on the bottom is because ispc doesn't write
# a target name into the dependency when creating multiple targets.  So,
# we write the target name in ourselves.
$(PGA_OBJ_DIR)/%.o: $(PGA_SRC_DIR)/%.ispc
	@mkdir -p $(dir $@)
	$(strip $(OP_ISPC) -o $@ $(PGA_ISPC_OPTS) \
		-M -MF $(@:.o=.d) \
		--target=$(subst $(PGA_SPACE),$(PGA_COMMA),$(OP_ISPC_TARGETS)) \
		--header-outfile=$(patsubst %,$(PGA_OBJ_DIR)/%,$(notdir $<.h)) $<)
	@sed -i -e s,\(null\),$@, $(@:.o=.d)

clean_spirent_pga: clean_spirent_pga_ispc
clean_spirent_pga_ispc:
	@rm -f $(PGA_ISPC_OBJECTS) $(PGA_ISPC_TARGET_OBJECTS)

$(PGA_TARGET): $(PGA_OBJECTS) $(PGA_ISPC_TARGET_OBJECTS) $(PGA_ISPC_OBJECTS)
	$(call op_link_library,$@,$(PGA_OBJECTS) $(PGA_ISPC_TARGET_OBJECTS))

.PHONY: spirent_pga
spirent_pga: $(PGA_TARGET)
