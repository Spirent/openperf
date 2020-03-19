ENV_DIR   		= env
SRC_DIR   		= protocol
DEF_DIR				= definitions
SCRIPT_DIR		= script
REQUIREMENTS	= $(SCRIPT_DIR)/requirements.txt
CODE_GEN			= $(SCRIPT_DIR)/code_generator.py
PROTOCOL_GEN	= $(SCRIPT_DIR)/make_protocol_header.sh
DEFS	  			= $(wildcard $(DEF_DIR)/*.yaml)
HEADERS   		= $(patsubst $(DEF_DIR)/%.yaml,$(SRC_DIR)/%.hpp,$(DEFS))
PROTOCOL_HDR	= $(SRC_DIR)/protocols.hpp

all: sources_clean sources

.PHONY: sources
sources: $(SRC_DIR) $(HEADERS) $(PROTOCOL_HDR)

$(SRC_DIR):
	mkdir -p $(SRC_DIR)

$(PROTOCOL_HDR): $(HEADERS)
	$(PROTOCOL_GEN) --dir $(SRC_DIR) --prefix packet

$(SRC_DIR)/%.hpp: $(DEF_DIR)/%.yaml | $(ENV_DIR)/
	$(ENV_DIR)/bin/python3 $(CODE_GEN) --infile $< --outdir $(SRC_DIR)

$(ENV_DIR)/: $(REQUIREMENTS)
	@rm -rf $(ENV_DIR)
	virtualenv -p python3 $(ENV_DIR)
	$(ENV_DIR)/bin/pip install -r $(REQUIREMENTS)

sources_clean:
	rm -rf $(SRC_DIR)

clean:
	rm -rf $(ENV_DIR)
