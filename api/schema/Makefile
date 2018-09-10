TOP_DIR		= ../..
OUTPUT_DIR	= $(TOP_DIR)/src/api
SWAGGER_CODEGEN	= $(TOP_DIR)/deps/swagger-codegen/swagger-codegen
SWAGGER_SPECS	= $(wildcard v*/*.yaml)

.PHONY: all generate

all:

generate: $(patsubst %.yaml,generate/%,$(SWAGGER_SPECS))

generate/%: %.yaml
	$(eval version=$(shell dirname $*))
	$(eval output_dir=$(OUTPUT_DIR)/$(version))
	$(eval model_pkg=icp.api.$(version).model)
	rm -rf $(OUTPUT_DIR)/model
	$(SWAGGER_CODEGEN) generate -Dmodels -DsupportingFiles -i $< -l pistache-server --additional-properties modelPackage=$(model_pkg) -o $(output_dir)
