#
# Makefile component for C++ digestible implementation
#

DIGESTIBLE_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(DIGESTIBLE_REQ_VARS))

DIGESTIBLE_SRC_DIR := $(OP_ROOT)/deps/digestible

OP_INC_DIRS += $(DIGESTIBLE_SRC_DIR)/include

###
# No build rules; header only
###
.PHONY: digestible
digestible:

.PHONY: clean_digestible
clean_digestible:
