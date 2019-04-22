CFGFILE_SOURCES += \
	config/config_file_utils.cpp \
	config/yaml_json_emitter.cpp

CFGFILE_DEPENDS += yaml_cpp

CFGFILETEST_SOURCES += \
	$(CFGFILE_SOURCES)

CFGFILETEST_DEPENDS += yaml_cpp
