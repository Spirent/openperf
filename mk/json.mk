#
# Makefile component for JSON for Modern c++
#

JSON_REQ_VARS := \
	ICP_ROOT
$(call icp_check_vars,$(JSON_REQ_VARS))

JSON_SRC_DIR := $(ICP_ROOT)/deps/json

ICP_INC_DIRS += $(JSON_SRC_DIR)/single_include/json

###
# No build rules; header only
###
.PHONY: json
json:

.PHONY: clean_json
clean_json:
