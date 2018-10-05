#
# Makefile component for CATCH Unit Testing framework
#

CATCH_REQ_VARS := \
	ICP_ROOT
$(call icp_check_vars,$(CATCH_REQ_VARS))

CATCH_SRC_DIR := $(ICP_ROOT)/deps/catch

ICP_INC_DIRS += $(CATCH_SRC_DIR)/include

###
# No build rules; header only
###
.PHONY: catch
catch:

.PHONY: clean_catch
clean_catch:
