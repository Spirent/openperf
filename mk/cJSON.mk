#
# Makefile component for cJSON
#

CJSON_SRC_DIR := $(ICP_ROOT)/deps/cJSON
CJSON_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/cJSON
CJSON_BLD_DIR := $(ICP_BUILD_ROOT)/cJSON
CJSON_INC_DIR := $(CJSON_BLD_DIR)/include
CJSON_LIB_DIR := $(CJSON_BLD_DIR)/lib

# Update global variables
ICP_INC_DIRS += $(CJSON_INC_DIR)
ICP_LIB_DIRS += $(CJSON_LIB_DIR)
ICP_LDLIBS += -lcJSON

###
# Build rules
###

CJSON_CFLAGS := -fPIC -std=c89 -pedantic -Wall -Werror -Wstrict-prototypes \
	-Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2 \
	-Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wc++-compat -Wundef \
	-Wswitch-default -Wconversion -fstack-protector

$(CJSON_OBJ_DIR)/cJSON.o: $(CJSON_SRC_DIR)/cJSON.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c -I$(CJSON_SRC_DIR) $(CJSON_CFLAGS) $(ICP_COPTS) $<)

$(CJSON_LIB_DIR)/libcJSON.a: $(CJSON_OBJ_DIR)/cJSON.o
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $<

$(CJSON_INC_DIR)/cjson/cJSON.h: $(CJSON_SRC_DIR)/cJSON.h
	@mkdir -p $(dir $@)
	@cp $< $@

.PHONY: cJSON
cJSON: $(CJSON_LIB_DIR)/libcJSON.a $(CJSON_INC_DIR)/cjson/cJSON.h

.PHONY: clean_cJSON
clean_cJSON:
	@rm -rf $(CJSON_OBJ_DIR) $(CJSON_BLD_DIR)
clean: clean_cJSON
