#
# Makefile component for packetio testing
#

PIOTEST_REQ_VARS := \
	ICP_ROOT
$(call icp_check_vars,$(PIOTEST_REQ_VARS))

PIOTEST_DEPENDS := immer
$(call icp_include_dependencies,$(PIOTEST_DEPENDS))

ICP_INC_DIRS += $(ICP_ROOT)/src/modules

###
# No build rules; header only for now
###
.PHONY: packetio_test
packetio_test:

.PHONY: clean_packetio_test
clean_packetio_test:
