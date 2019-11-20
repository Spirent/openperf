#
# Makefile component for YAML C++ library
#

YAML_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(YAML_REQ_VARS))

YAML_SRC_DIR := $(OP_ROOT)/deps/yaml-cpp
YAML_OBJ_DIR := $(OP_BUILD_ROOT)/obj/yaml-cpp
YAML_BLD_DIR := $(OP_BUILD_ROOT)/yaml-cpp
YAML_INC_DIR := $(YAML_SRC_DIR)/include
YAML_LIB_DIR := $(YAML_BLD_DIR)/lib

# Update global variables
OP_INC_DIRS += $(YAML_INC_DIR)
OP_LIB_DIRS += $(YAML_LIB_DIR)
OP_LDLIBS += -lyamlcpp

YAML_FLAGS := $(addprefix -I,$(YAML_INC_DIR))
# YAML-CPP shadows variables in a few places; just silence those warnings since we're not going
# to fix them
YAML_FLAGS += -Wno-shadow

###
# Sources, objects, targets, and dependencies
###
YAML_SOURCES := \
	src/binary.cpp \
	src/convert.cpp	\
	src/contrib/graphbuilder.cpp \
	src/contrib/graphbuilderadapter.cpp \
	src/directives.cpp \
	src/emit.cpp \
	src/emitfromevents.cpp \
	src/emitter.cpp \
	src/emitterstate.cpp \
	src/emitterutils.cpp \
	src/exceptions.cpp \
	src/exp.cpp \
	src/memory.cpp \
	src/nodebuilder.cpp \
	src/node.cpp \
	src/node_data.cpp \
	src/nodeevents.cpp \
	src/null.cpp \
	src/ostream_wrapper.cpp \
	src/parse.cpp \
	src/parser.cpp \
	src/regex_yaml.cpp \
	src/scanner.cpp \
	src/scanscalar.cpp \
	src/scantag.cpp \
	src/scantoken.cpp \
	src/simplekey.cpp \
	src/singledocparser.cpp \
	src/stream.cpp \
	src/tag.cpp

YAML_OBJECTS := $(call op_generate_objects,$(YAML_SOURCES),$(YAML_OBJ_DIR))

# Pull in object dependencies, maybe
-include $(YAML_OBJECTS:.o=.d)

YAML_TARGET := $(YAML_LIB_DIR)/libyamlcpp.a

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(YAML_SOURCES),YAML_SRC_DIR,YAML_OBJ_DIR,,YAML_FLAGS))
$(eval $(call op_generate_clean_rules,yaml_cpp,YAML_TARGET,YAML_OBJECTS))

$(YAML_TARGET): $(YAML_OBJECTS)
	$(call op_link_library,$@,$(YAML_OBJECTS))

.PHONY: yaml_cpp
yaml_cpp: $(YAML_TARGET)
