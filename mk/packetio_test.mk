#
# Makefile component for packetio testing
#

PIOTEST_REQ_VARS := \
	OP_ROOT
$(call op_check_vars,$(PIOTEST_REQ_VARS))

PIOTEST_DEPENDS := immer
$(call op_include_dependencies,$(PIOTEST_DEPENDS))

OP_INC_DIRS += $(OP_ROOT)/src/modules

###
# No build rules; header only for now
###
.PHONY: packetio_test
packetio_test:

.PHONY: clean_packetio_test
clean_packetio_test:
