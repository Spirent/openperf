#
# Makefile component for YAML based configuration files
#

CFGFILE_DEPENDS += yaml_cpp

CFGFILE_SOURCES += \
	config/op_config_file.cpp \
	config/op_config_file_options.c \
	config/op_config_file_utils.cpp \
	config/op_config_utils.cpp \
	config/yaml_json_emitter.cpp
