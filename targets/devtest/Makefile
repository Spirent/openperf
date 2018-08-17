#
# Makefile to build devtest
#

ICP_ROOT := $(realpath ../../)
ICP_TARGET := devtest
include $(ICP_ROOT)/mk/bootstrap.mk

ICP_DPDK_TARGET := default-linuxicp-clang

BUILD_DIR := $(ICP_BUILD_ROOT)/obj
TARGET_DIR := $(ICP_BUILD_ROOT)/bin

SOURCES := main.c
OBJECTS := $(patsubst %, $(BUILD_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(SOURCES))))
SRC_DEPENDS := $(OBJECTS:.o=.d)
BLD_DEPENDS := dpdk libzmq cJSON civetweb

all: devtest

# Pull in dependencies
-include $(SRC_DEPENDS)
$(foreach DEP,$(BLD_DEPENDS),$(eval include $(ICP_ROOT)/mk/$(DEP).mk))

$(OBJECTS): | $(BLD_DEPENDS)

ICP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
ICP_CPPFLAGS += $(addprefix -I,$(ICP_INC_DIRS))
ICP_LDFLAGS += $(addprefix -L,$(ICP_LIB_DIRS))
ICP_LDOPTS += -static-libstdc++ -static-libgcc

# Build rule
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(TARGET_DIR)/devtest: $(OBJECTS)
	$(info $(TARGET_DIR))
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ $(ICP_LDOPTS) $(ICP_LDFLAGS) $(OBJECTS) $(ICP_LDLIBS))

.PHONY: devtest
devtest: $(TARGET_DIR)/devtest

.PHONY: clean_devtest
clean_devtest:
	@$(strip rm -f $(TARGET_DIR)/devtest $(OBJECTS:%.o=%.[doP]))
clean: clean_devtest
