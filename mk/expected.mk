#
# Makefile component for C++ expected implementation
#

EXPECTED_REQ_VARS := \
	ICP_ROOT
$(call icp_check_vars,$(EXPECTED_REQ_VARS))

EXPECTED_SRC_DIR := $(ICP_ROOT)/deps/expected

ICP_INC_DIRS += $(EXPECTED_SRC_DIR)

###
# No build rules; header only
###
.PHONY: expected
expected:

.PHONY: clean_expected
clean_expected:
