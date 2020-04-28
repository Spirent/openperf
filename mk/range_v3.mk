#
# Makefile component for Range-v3
#

RANGE_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(RANGE_REQ_VARS))

RANGE_SRC_DIR := $(OP_ROOT)/deps/range-v3

OP_INC_DIRS += $(RANGE_SRC_DIR)/include

###
# No build rules; header only
###
.PHONY: range_v3
range_v3:

.PHONY: clean_range_v3
clean_range_v3:
