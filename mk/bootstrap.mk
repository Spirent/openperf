#
# Main makefile component
#

###
# Useful build "functions"
###
icp_check_var =	$(if $(value $1),,$(error $(1) is not set))
icp_check_vars = $(foreach var,$1,$(call icp_check_var,$(var)))

# Initialize global variables
ICP_AR := gcc-ar
ICP_ARFLAGS := rcD
ICP_CC := clang
ICP_CXX := clang++
ICP_COPTS :=
ICP_CFLAGS := -std=gnu11
ICP_CPPFLAGS := -Wall -Wextra -Wpedantic -Wshadow -Wno-gnu \
	-MMD -MP -fstack-protector-strong
ICP_CXXFLAGS :=
ICP_DEFINES :=
ICP_LDFLAGS :=
ICP_LDLIBS :=
ICP_LDOPTS :=
ICP_INC_DIRS :=
ICP_LIB_DIRS :=
ICP_CONFIG_OPTS :=

# Detect and update build environment flags
$(call icp_check_vars,ICP_ROOT ICP_TARGET)
include $(ICP_ROOT)/mk/build.mk
