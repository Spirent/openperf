#
# Makefile component for base-n
#

RANGE_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(RANGE_REQ_VARS))

RANGE_SRC_DIR := $(OP_ROOT)/deps/base-n

OP_INC_DIRS += $(RANGE_SRC_DIR)/include

###
# No build rules; header only
###
.PHONY: base_n
base_n:

.PHONY: clean_base_n
clean_base_n:
