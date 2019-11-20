#
# Main makefile component
#

###
# Useful build "functions"
###
op_check_var =	$(if $(value $1),,$(error $(1) is not set))
op_check_vars = $(foreach var,$1,$(call op_check_var,$(var)))

# Initialize global variables
CC := clang
CXX := clang++

OP_AR := llvm-ar
OP_ARFLAGS := rcs
OP_CC := $(CC) -flto=thin
OP_CXX := $(CXX) -flto=thin
OP_COPTS := -march=native
OP_CSTD := -std=gnu11  # c++11 with GNU extensions
OP_CXXSTD := -std=c++17
OP_CFLAGS := $(OP_CSTD)
OP_CPPFLAGS := -Wall -Werror -Wextra -Wpedantic -Wshadow -Wno-gnu \
	-MMD -MP -fstack-protector-strong
OP_CXXFLAGS := -Wno-c11-extensions -Wno-nested-anon-types -Wno-c99-extensions $(OP_CXXSTD)
OP_DEFINES :=
OP_LDFLAGS :=
OP_LDLIBS :=
OP_LDOPTS := -fuse-ld=lld
OP_INC_DIRS := $(OP_ROOT)/src
OP_LIB_DIRS :=
OP_CONFIG_OPTS :=

OP_BUILD_DEPENDENCIES :=

# Detect and update build environment flags
$(call op_check_vars,OP_ROOT OP_TARGET)
include $(OP_ROOT)/mk/build.mk

# Load useful make templates
include $(OP_ROOT)/mk/templates.mk
