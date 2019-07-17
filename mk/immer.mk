#
# Makefile component for immer, immutable data structures for c++
#

IMMER_REQ_VARS := \
	ICP_ROOT
$(call icp_check_vars,$(IMMER_REQ_VARS))

IMMER_SRC_DIR := $(ICP_ROOT)/deps/immer

ICP_INC_DIRS += $(IMMER_SRC_DIR)

###
# No build rules; header only
###
.PHONY: immer
immer:

.PHONY: clean_immer
clean_immer:
