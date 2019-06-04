CFGFILE_SOURCES += \
	config/icp_config_file.cpp \
	config/icp_config_file_options.c \
	config/icp_config_file_utils.cpp \
	config/yaml_json_emitter.cpp

CFGFILE_DEPENDS += yaml_cpp

CFGFILETEST_SOURCES += \
	$(CFGFILE_SOURCES)

CFGFILETEST_DEPENDS += yaml_cpp
