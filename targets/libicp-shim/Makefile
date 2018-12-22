#
# Makefile to build inception shim library
#

ICP_ROOT := $(realpath ../../)
ICP_TARGET := libicp-shim
include $(ICP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(ICP_BUILD_ROOT)/obj
TARGET_DIR := $(ICP_BUILD_ROOT)/lib

SOURCES := \
	icp-shim.cpp \
	libc_wrapper.cpp

OBJECTS := $(patsubst %, $(BUILD_DIR)/%, \
        $(patsubst %.cpp, %.o, $(filter %.cpp, $(SOURCES))))
SRC_DEPENDS := $(OBJECTS:.o=.d)

BLD_DEPENDS := socket_client

TARGET := $(TARGET_DIR)/$(ICP_TARGET).so

all: $(TARGET)

# Pull in dependencies
-include $(SRC_DEPENDS)
$(foreach DEP,$(BLD_DEPENDS),$(eval include $(ICP_ROOT)/mk/$(DEP).mk))

$(OBJECTS): | $(BLD_DEPENDS)

ICP_CPPFLAGS += -fPIC $(addprefix -I,$(ICP_INC_DIRS))
ICP_LDFLAGS += $(addprefix -L,$(ICP_LIB_DIRS))
ICP_LDOPTS += -static-libstdc++ -static-libgcc -shared
ICP_LDLIBS += -ldl

# Build rules
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(TARGET): $(OBJECTS) $(BLD_DEPENDS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ $(ICP_LDOPTS) $(ICP_LDFLAGS) $(OBJECTS) $(ICP_LDLIBS))

.PHONY: $(ICP_TARGET)
$(ICP_TARGET): $(TARGET)

.PHONY: clean_$(ICP_TARGET)
clean_$(ICP_TARGET):
	@$(strip rm -rf $(TARGET_DIR) $(BUILD_DIR))
clean: clean_$(ICP_TARGET)

clean:
	@$(strip rm -rf $(ICP_BUILD_ROOT))
