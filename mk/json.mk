#
# Makefile component for JSON for Modern c++
#

JSON_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(JSON_REQ_VARS))

JSON_SRC_DIR := $(OP_ROOT)/deps/json

OP_INC_DIRS += $(JSON_SRC_DIR)/single_include/nlohmann
OP_CPPFLAGS += -Wno-tautological-overlap-compare  # Needed for clang-10

###
# No build rules; header only
###
.PHONY: json
json:

.PHONY: clean_json
clean_json:
