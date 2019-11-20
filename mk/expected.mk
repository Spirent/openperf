#
# Makefile component for C++ expected implementation
#

EXPECTED_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(EXPECTED_REQ_VARS))

EXPECTED_SRC_DIR := $(OP_ROOT)/deps/expected

OP_INC_DIRS += $(EXPECTED_SRC_DIR)

###
# No build rules; header only
###
.PHONY: expected
expected:

.PHONY: clean_expected
clean_expected:
