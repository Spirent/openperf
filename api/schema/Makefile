TOP_DIR			= ../..
SRC_OUTPUT_DIR		= $(TOP_DIR)/src/swagger
AAT_DIR			= $(TOP_DIR)/tests/aat
AAT_TEMPLATE_DIR	= $(AAT_DIR)/template
AAT_OUTPUT_DIR		= $(AAT_DIR)/api
SWAGGER_CODEGEN		= $(TOP_DIR)/deps/swagger-codegen/swagger-codegen
SWAGGER_SPECS		= $(wildcard v*/openperf.yaml)

SWAGGER_VERSIONS = $(patsubst %/,%,$(sort $(dir $(SWAGGER_SPECS))))

all: generate

generate: $(addprefix reset_dirs/,$(SWAGGER_VERSIONS)) $(patsubst %.yaml,generate/%,$(SWAGGER_SPECS))

reset_dirs/%: $(SWAGGER_VERSIONS)
	$(eval version=$(notdir $@))
	$(eval src_output_dir=$(SRC_OUTPUT_DIR)/$(version))
	$(eval src_ignore_file=$(src_output_dir)/.swagger-codegen-ignore)
	rm -rf $(src_output_dir)
	mkdir -p $(dir $(src_ignore_file))
	echo "CMakeLists.txt" > $(src_ignore_file)
	echo "README.md" >> $(src_ignore_file)

	$(eval aat_output_dir=$(AAT_OUTPUT_DIR)/$(version))
	$(eval aat_ignore_file=$(aat_output_dir)/.swagger-codegen-ignore)
	rm -rf $(aat_output_dir)
	mkdir -p $(dir $(aat_ignore_file))
	echo ".travis.yml" > $(aat_ignore_file)
	echo "README.md" >> $(aat_ignore_file)
	echo "git_push.sh" >> $(aat_ignore_file)
	echo "test-requirements.txt" >> $(aat_ignore_file)
	echo "tox.ini" >> $(aat_ignore_file)

generate/%: %.yaml
	$(eval version=$(shell dirname $*))
	$(eval src_output_dir=$(SRC_OUTPUT_DIR)/$(version))
	$(eval src_model_pkg=swagger.$(version).model)
	$(SWAGGER_CODEGEN) generate -Dmodels -DsupportingFiles -i $< -l pistache-server \
		--additional-properties modelPackage=$(src_model_pkg) -o $(src_output_dir)

	$(eval aat_output_dir=$(AAT_OUTPUT_DIR)/$(version))
	$(SWAGGER_CODEGEN) generate -DapiTests=false -DmodelTests=false -i $< -l python \
		-t $(AAT_TEMPLATE_DIR) --additional-properties packageName=client -o $(aat_output_dir)
