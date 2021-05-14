REGURGITATE_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT \
	OP_ISPC \
	OP_ISPC_TARGETS
$(call op_check_vars,$(REGURGITATE_REQ_VARS))

# Need these to generate a comma separated target list for ISPC
REGURGITATE_COMMA := ,
REGURGITATE_EMPTY :=
REGURGITATE_SPACE := $(REGURGITATE_EMPTY) $(REGURGITATE_EMPTY)
REGURGITATE_ISPC_FLAGS := $(OP_ISPC_FLAGS)

ifeq ($(MODE),release)
	REGURGITATE_ISPC_FLAGS += --opt=disable-assertions
else
	REGURGITATE_ISPC_FLAGS += -g
	ifeq ($(MODE),debug)
		REGURGITATE_ISPC_FLAGS += -O0
	endif
endif

REGURGITATE_SRC_DIR := $(OP_ROOT)/src/lib/regurgitate
REGURGITATE_INC_DIR := $(dir $(REGURGITATE_SRC_DIR))
REGURGITATE_OBJ_DIR := $(OP_BUILD_ROOT)/obj/lib/regurgitate
REGURGITATE_LIB_DIR := $(OP_BUILD_ROOT)/lib

REGURGITATE_SOURCES :=

include $(REGURGITATE_SRC_DIR)/directory.mk

REGURGITATE_OBJECTS := $(call op_generate_objects,$(filter-out %.ispc,$(REGURGITATE_SOURCES)),$(REGURGITATE_OBJ_DIR))

###
# ISPC generates multiple object files when using multiple targets.
# Translate targets and options into proper object names
###
REGURGITATE_ISPC_HEADERS := $(patsubst %, $(REGURGITATE_OBJ_DIR)/%, \
	$(notdir $(patsubst %.ispc, %.ispc.h, $(filter %.ispc, $(REGURGITATE_SOURCES)))))

REGURGITATE_ISPC_OBJECTS := $(patsubst %, $(REGURGITATE_OBJ_DIR)/%, \
	$(patsubst %.ispc, %.o, $(filter %.ispc, $(REGURGITATE_SOURCES))))

# Adjust objects based on the number of ISPC target architectures
# Since we don't have any x16 wide implementations, we filter those targets out
REGURGITATE_DEFINES :=
REGURGITATE_ISPC_TARGETS := $(filter-out %-i32x16,$(OP_ISPC_TARGETS))
REGURGITATE_ISPC_TARGET_OBJECTS :=

ifeq (,$(word 2,$(REGURGITATE_ISPC_TARGETS)))  # i.e. there is no 2nd target
	REGURGITATE_DEFINES += ISPC_TARGET_AUTOMATIC
	REGURGITATE_ISPC_TARGET_OBJECTS += $(REGURGITATE_ISPC_OBJECTS)
else
	ifneq (,$(filter sse2-%,$(REGURGITATE_ISPC_TARGETS)))
		REGURGITATE_DEFINES += ISPC_TARGET_SSE2
		REGURGITATE_ISPC_TARGET_OBJECTS += $(addsuffix _sse2.o,$(basename $(REGURGITATE_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter sse4-%,$(REGURGITATE_ISPC_TARGETS)))
		REGURGITATE_DEFINES += ISPC_TARGET_SSE4
		REGURGITATE_ISPC_TARGET_OBJECTS += $(addsuffix _sse4.o,$(basename $(REGURGITATE_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx1-%,$(REGURGITATE_ISPC_TARGETS)))
		REGURGITATE_DEFINES += ISPC_TARGET_AVX
		REGURGITATE_ISPC_TARGET_OBJECTS += $(addsuffix _avx.o,$(basename $(REGURGITATE_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx2-%,$(REGURGITATE_ISPC_TARGETS)))
		REGURGITATE_DEFINES += ISPC_TARGET_AVX2
		REGURGITATE_ISPC_TARGET_OBJECTS += $(addsuffix _avx2.o,$(basename $(REGURGITATE_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter avx512skx-%,$(REGURGITATE_ISPC_TARGETS)))
		REGURGITATE_DEFINES += ISPC_TARGET_AVX512SKX
		REGURGITATE_ISPC_TARGET_OBJECTS += $(addsuffix _avx512skx.o,$(basename $(REGURGITATE_ISPC_OBJECTS)))
	endif
	ifneq (,$(filter neon-%,$(REGURGITATE_ISPC_TARGETS)))
		REGURGITATE_DEFINES += ISPC_TARGET_NEON
		REGURGITATE_ISPC_TARGET_OBJECTS += $(addsuffix _neon.o,$(basename $(REGURGITATE_ISPC_OBJECTS)))
	endif

# The ISPC target objects depend on their source "objects"
$(REGURGITATE_ISPC_TARGET_OBJECTS): $(REGURGITATE_ISPC_OBJECTS)
endif

# Turn defines into proper compiler flags
REGURGITATE_FLAGS := $(addprefix -D,$(REGURGITATE_DEFINES))

REGURGITATE_LIBRARY := openperf_regurgitate
REGURGITATE_TARGET := $(REGURGITATE_LIB_DIR)/lib$(REGURGITATE_LIBRARY).a

OP_INC_DIRS += $(REGURGITATE_INC_DIR)
OP_LIB_DIRS += $(REGURGITATE_LIB_DIR)
OP_LDLIBS += -l$(REGURGITATE_LIBRARY)

-include $(REGURGITATE_OBJECTS:.o=.d)
-include $(REGURGITATE_ISPC_OBJECTS:.o=.d)

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(REGURGITATE_SOURCES),REGURGITATE_SRC_DIR,REGURGITATE_OBJ_DIR,,REGURGITATE_FLAGS))
$(eval $(call op_generate_clean_rules,regurgitate,REGURGITATE_TARGET,REGURGITATE_OBJECTS))

# In addition to the standard rules above, we need a couple of ISPC rules
# Note: that the sed command on the bottom is because ispc doesn't write
# a target name into the dependency when creating multiple targets.  So,
# we write the target name in ourselves.
$(REGURGITATE_OBJ_DIR)/%.o: $(REGURGITATE_SRC_DIR)/%.ispc
	@mkdir -p $(dir $@)
	$(strip $(OP_ISPC) -o $@ $(REGURGITATE_ISPC_FLAGS) \
		-M -MF $(@:.o=.d) \
		--target=$(subst $(REGURGITATE_SPACE),$(REGURGITATE_COMMA),$(REGURGITATE_ISPC_TARGETS)) \
		--header-outfile=$(patsubst %,$(REGURGITATE_OBJ_DIR)/%,$(notdir $<.h)) $<)
	@sed -i -e s,\(null\),$@, $(@:.o=.d)

clean_regurgitate: clean_regurgitate_ispc
clean_regurgitate_ispc:
	@rm -f $(REGURGITATE_ISPC_OBJECTS) $(REGURGITATE_ISPC_TARGET_OBJECTS)

$(REGURGITATE_TARGET): $(REGURGITATE_OBJECTS) $(REGURGITATE_ISPC_TARGET_OBJECTS) $(REGURGITATE_ISPC_OBJECTS)
	$(call op_link_library,$@,$(REGURGITATE_OBJECTS) $(REGURGITATE_ISPC_TARGET_OBJECTS))

.PHONY: regurgitate
regurgitate: $(REGURGITATE_TARGET)
