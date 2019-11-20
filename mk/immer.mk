#
# Makefile component for immer, immutable data structures for c++
#

IMMER_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(IMMER_REQ_VARS))

IMMER_SRC_DIR := $(OP_ROOT)/deps/immer

OP_INC_DIRS += $(IMMER_SRC_DIR)

###
# No build rules; header only
###
.PHONY: immer
immer:

.PHONY: clean_immer
clean_immer:
