#
# Makefile to build stack
#

ICP_ROOT := $(realpath ../../)
ICP_TARGET := stack
include $(ICP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(ICP_BUILD_ROOT)/obj
TARGET_DIR := $(ICP_BUILD_ROOT)/bin

SOURCES := \
	main.c

OBJECTS := $(patsubst %, $(BUILD_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(SOURCES))))
SRC_DEPENDS := $(OBJECTS:.o=.d)

BLD_DEPENDS := api framework packetio
ICP_PACKETIO_DRIVER := dpdk

all: inception

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

$(TARGET_DIR)/inception: $(OBJECTS) $(BLD_DEPENDS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ $(ICP_LDOPTS) $(ICP_LDFLAGS) $(OBJECTS) $(ICP_LDLIBS))
	- sudo -n /sbin/setcap CAP_IPC_LOCK,CAP_NET_RAW,CAP_SYS_RAWIO,CAP_IPC_OWNER,CAP_NET_BIND_SERVICE,CAP_SETUID,CAP_SYS_ADMIN,CAP_FOWNER,CAP_DAC_OVERRIDE=epi $@ || true

.PHONY: inception
inception: $(TARGET_DIR)/inception

.PHONY: clean_inception
clean_inception:
	@$(strip rm -f $(TARGET_DIR)/inception $(OBJECTS:%.o=%.[doP]))
clean: clean_inception
