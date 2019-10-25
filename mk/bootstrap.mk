#
# Main makefile component
#

###
# Useful build "functions"
###
icp_check_var =	$(if $(value $1),,$(error $(1) is not set))
icp_check_vars = $(foreach var,$1,$(call icp_check_var,$(var)))

# Initialize global variables
CC := clang
CXX := clang++

ICP_AR := llvm-ar
ICP_ARFLAGS := rcs
ICP_CC := $(CC) -flto=thin
ICP_CXX := $(CXX) -flto=thin
ICP_COPTS := -march=native
ICP_CSTD := -std=gnu11  # c++11 with GNU extensions
ICP_CXXSTD := -std=c++17
ICP_CFLAGS := $(ICP_CSTD)
ICP_CPPFLAGS := -Wall -Werror -Wextra -Wpedantic -Wshadow -Wno-gnu \
	-MMD -MP -fstack-protector-strong
ICP_CXXFLAGS := -Wno-c11-extensions -Wno-nested-anon-types -Wno-c99-extensions $(ICP_CXXSTD)
ICP_DEFINES :=
ICP_LDFLAGS :=
ICP_LDLIBS :=
ICP_LDOPTS := -fuse-ld=lld
ICP_INC_DIRS := $(ICP_ROOT)/src
ICP_LIB_DIRS :=
ICP_CONFIG_OPTS :=

ICP_BUILD_DEPENDENCIES :=

# Detect and update build environment flags
$(call icp_check_vars,ICP_ROOT ICP_TARGET)
include $(ICP_ROOT)/mk/build.mk

# Load useful make templates
include $(ICP_ROOT)/mk/templates.mk
