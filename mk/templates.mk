#
# Templates for common inception/makefile boilerplate
#

###
# Helper functions
###

# Include a dependency iff it has not been included before.
# We use a predefined tracking variable to ensure we only include a dependency
# once.
define icp_include_dependency
ifeq (,$(findstring $(1),$(ICP_BUILD_DEPENDENCIES)))
include $(ICP_ROOT)/mk/$(1).mk
ICP_BUILD_DEPENDENCIES += $(1)
endif
endef

# Include pre-defined dependencies
# $(1): list of dependencies
icp_include_dependencies = $(foreach item,$(1),$(eval $(call icp_include_dependency,$(item))))

# Translate all sources files with the given extension to object files
# $(1): source file extension
# $(2): source files
# $(3): build directory
icp_objects_template = $(patsubst %, $(3)/%, $(patsubst $(join %,$(1)), %.o, $(filter $(join %,$(1)), $(2))))

# Generate a list of objects from a list of source files
# $(1): source file list
# $(2): build directory
icp_generate_objects = $(foreach ext,$(sort $(suffix $(1))),$(call icp_objects_template,$(ext),$(1),$(2)))

# Make sure the build rule path has the right number of slashes
icp_build_rule_src_path =\
	$(if $(filter undefined,$(flavor $(1))),$(2),$(subst //,/,$$($(1))/$(2)))

###
# Build functions
###

# Standard build rule for C source files
# $(1): source code suffix
# $(2): path to sources
# $(3): variable containing build directory path
# $(4): variable containing build dependencies
# $(5): variable containing build specific compiler options
define icp_c_build_rule_template
$$($(3))/%.o: $(call icp_build_rule_src_path,$(2),%$(1)) | $$($(4))
	@mkdir -p $$(dir $$@)
	$$(strip $$(ICP_CC) -o $$@ -c $$(ICP_CPPFLAGS) $$(ICP_CFLAGS) $$(ICP_COPTS) $$(ICP_DEFINES) $$($(5)) $$<)

endef

# Standard build rule for C++ source files
# $(1): source code suffix
# $(2): path to sources
# $(3): variable containing build directory path
# $(4): variable containing build dependencies
# $(5): variable containing build specific compiler options
define icp_cxx_build_rule_template
$$($(3))/%.o: $(call icp_build_rule_src_path,$(2),%$(1)) | $$($(4))
	@mkdir -p $$(dir $$@)
	$$(strip $$(ICP_CXX) -o $$@ -c $$(ICP_CPPFLAGS) $$(ICP_CXXFLAGS) $$(ICP_COPTS) $$(ICP_DEFINES) $$($(5)) $$<)

endef

# What monster would ever use anything different?
C_SRC_EXTENSIONS := .c
CXX_SRC_EXTENSIONS := .cc .cpp .cxx .c++

# Filter and sort the source file list to determine the actual extensions used in the
# list of source files.  For every extension that matches one of our know extensions,
# generate a build rule using the appropriate template
# $(1): list of sources
# $(2): source directory
# $(3): object directory
# $(4): build dependencies
# $(5): compiler flags
# Note: function must be called with $(eval)
define icp_generate_build_rules
$(foreach ext,$(sort $(suffix $(filter $(addprefix %,$(C_SRC_EXTENSIONS)),$(1)))),$(call icp_c_build_rule_template,$(ext),$(2),$(3),$(4),$(5)))
$(foreach ext,$(sort $(suffix $(filter $(addprefix %,$(CXX_SRC_EXTENSIONS)),$(1)))),$(call icp_cxx_build_rule_template,$(ext),$(2),$(3),$(4),$(5)))
endef

###
# Clean functions
###

# Standard clean rule
# $(1) variable with target name
# $(2) variable with full name of actual target
# $(3) variable with build objects
define icp_clean_target_var_template
.PHONY: clean_$$($(1))
clean_$$($(1)):
	@$$(strip rm -f $$($(2)) $$($(3):%.o=%.[doP]))
clean: clean_$$($(1))
endef

# Standard clean rule
# $(1) target name
# $(2) variable with full name of actual target
# $(3) variable with build objects
define icp_clean_target_named_template
.PHONY: clean_$(1)
clean_$(1):
	@$$(strip rm -f $$($(2)) $$($(3):%.o=%.[doP]))
endef

# Wrapper to handoff to proper clean rule template
# $(1) target name, either as text or variable
# $(2) variable with full name of actual target
# $(3) variable with build objects
# Note: function must be called with $(eval)
icp_generate_clean_rules = \
	$(if $(filter undefined,$(flavor $(1))),\
		$(call icp_clean_target_named_template,$(1),$(2),$(3)),\
		$(call icp_clean_target_var_template,$(1),$(2),$(3)))

###
# Linking functions
###

# Standard command to generate a binary
# $(1): binary name
# $(2): binary objects
# $(3): extra link options (optional)
define icp_link_binary
	@mkdir -p $(dir $(1))
	$(strip $(ICP_CXX) -o $(1) $(ICP_LDOPTS) $(ICP_LDFLAGS) $(2) $(ICP_LDLIBS) $(3))
endef

define icp_link_shared_library
	$(strip $(ICP_CXX) -shared -o $@ $(ICP_LDOPTS) $(ICP_LDFLAGS) $(3) $(2) $(ICP_LDLIBS))
endef

define icp_link_static_library
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $(3) $(1) $(2))
endef

STATIC_LIB_EXTENSIONS := .a
SHARED_LIB_EXTENSIONS := .so .dylib

# Wrapper to handoff to appropriate library link command based on the
# library name
icp_library_link_filter = \
	$(if $(filter $(addprefix %,$(SHARED_LIB_EXTENSIONS)),$(1)),\
		$(call icp_link_shared_library,$(1),$(2),$(3)),\
		$(call icp_link_static_library,$(1),$(2),$(3)))

# Standard library creation command
# $(1): library name
# $(2): library objects
# $(3): extra link options (optional)
define icp_link_library
	@mkdir -p $(dir $(1))
	$(call icp_library_link_filter,$(1),$(2),$(3))
endef
