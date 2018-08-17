#
# Makefile component containing helpful rules and functions
#

define ADD_CLEAN_RULE
	.PHONY clean_${1}
	clean_${1}:
		@echo "Clean ${1}"
	clean: clean_${1}
endef
