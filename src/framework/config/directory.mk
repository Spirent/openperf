#
# Makefile component for YAML based configuration files
#

CFGFILE_DEPENDS += yaml_cpp

CFGFILE_SOURCES += \
	op_config_file.cpp \
	op_config_file_options.c \
	op_config_file_utils.cpp \
	op_config_utils.cpp \
	yaml_json_emitter.cpp
