#
# Makefile component for JSON for Modern c++
#

JSON_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(JSON_REQ_VARS))

JSON_SRC_DIR := $(OP_ROOT)/deps/json

OP_INC_DIRS += $(JSON_SRC_DIR)/single_include/nlohmann

###
# No build rules; header only
###
.PHONY: json
json:

.PHONY: clean_json
clean_json:
