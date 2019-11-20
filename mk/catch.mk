#
# Makefile component for CATCH Unit Testing framework
#

CATCH_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(CATCH_REQ_VARS))

CATCH_SRC_DIR := $(OP_ROOT)/deps/catch

OP_INC_DIRS += $(CATCH_SRC_DIR)/include

###
# No build rules; header only
###
.PHONY: catch
catch:

.PHONY: clean_catch
clean_catch:
