TOP_DIR		= ../..
OUTPUT_DIR	= $(TOP_DIR)/src/model
SWAGGER_CODEGEN	= $(TOP_DIR)/deps/swagger-codegen/swagger-codegen
SWAGGER_SPECS	= $(wildcard v*/*.yaml)

.PHONY: all generate

all:

generate: $(patsubst %.yaml,generate/%,$(SWAGGER_SPECS))

generate/%: %.yaml
	$(eval version=$(shell dirname $*))
	$(eval temp_dir=$(shell mktemp -d))
	$(eval output_dir=$(OUTPUT_DIR)/$(version))
	$(eval model_pkg=icp.server.model.$(version))
	$(SWAGGER_CODEGEN) generate -Dmodels -DsupportingFiles -i $< -l pistache-server --additional-properties modelPackage=$(model_pkg) -o $(temp_dir)
	rm -rf $(output_dir)
	mv $(temp_dir)/model $(output_dir)
	rm -rf $(temp_dir)
