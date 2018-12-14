#
# Makefile to build stack
#

ICP_ROOT := $(realpath ../../)
ICP_TARGET := client
include $(ICP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(ICP_BUILD_ROOT)/obj
TARGET_DIR := $(ICP_BUILD_ROOT)/bin

SOURCES := \
	main.cpp

OBJECTS := $(patsubst %, $(BUILD_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(SOURCES))))
SRC_DEPENDS := $(OBJECTS:.o=.d)

BLD_DEPENDS := socket_client

all: devclient

# Pull in dependencies
-include $(SRC_DEPENDS)
$(foreach DEP,$(BLD_DEPENDS),$(eval include $(ICP_ROOT)/mk/$(DEP).mk))

$(OBJECTS): | $(BLD_DEPENDS)

ICP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
ICP_CPPFLAGS += $(addprefix -I,$(ICP_INC_DIRS))
ICP_LDFLAGS += $(addprefix -L,$(ICP_LIB_DIRS))
ICP_LDOPTS += -static-libstdc++ -static-libgcc

# Build rule
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(TARGET_DIR)/devclient: $(OBJECTS) $(BLD_DEPENDS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ $(ICP_LDOPTS) $(ICP_LDFLAGS) $(OBJECTS) $(ICP_LDLIBS))

.PHONY: devclient
devclient: $(TARGET_DIR)/devclient

.PHONY: clean_devclient
clean_devclient:
	@$(strip rm -f $(TARGET_DIR)/devclient $(OBJECTS:%.o=%.[doP]))
clean: clean_devclient
