ROOT					= ../../../..
ENV_DIR				= env
SCRIPT_DIR		= script
SRC_DIR			= transmogrify

PKT_DEF_DIR		= $(ROOT)/src/lib/packet/definitions
PKT_DEFS			= $(wildcard $(PKT_DEF_DIR)/*.yaml)
HEADERS				= $(patsubst $(PKT_DEF_DIR)/%.yaml,$(SRC_DIR)/%.hpp,$(PKT_DEFS))

REQUIREMENTS	= $(SCRIPT_DIR)/requirements.txt
CODE_GEN			= $(SCRIPT_DIR)/transmogrify_generator.py
SWAGGER_GEN		= $(SCRIPT_DIR)/swagger_generator.py
SWAGGER_DIR		= $(ROOT)/api/schema/v1/modules/packet
SWAGGER_DEF		= $(SWAGGER_DIR)/protocol.yaml

all: definition_clean source_clean definition source

.PHONY: definition
definition: $(SWAGGER_DEF)

$(SWAGGER_DEF): $(PKT_DEFS) | $(ENV_DIR)/
	$(ENV_DIR)/bin/python3 $(SWAGGER_GEN) --indir $(PKT_DEF_DIR) --outfile $@

.PHONY: source
source: $(SRC_DIR) $(HEADERS)

$(SRC_DIR):
	@mkdir -p $(SRC_DIR)

$(SRC_DIR)/%.hpp: $(PKT_DEF_DIR)/%.yaml | $(ENV_DIR)
	$(ENV_DIR)/bin/python3 $(CODE_GEN) --infile $< --outdir $(SRC_DIR)

$(ENV_DIR)/: $(REQUIREMENTS)
	@rm -rf $(ENV_DIR)
	virtualenv -p python3 $(ENV_DIR)
	$(ENV_DIR)/bin/pip install -r $(REQUIREMENTS)

definition_clean:
	rm -rf $(SWAGGER_DEF)

source_clean:
	rm -rf $(SRC_DIR)

clean:
	rm -rf $(ENV_DIR)
