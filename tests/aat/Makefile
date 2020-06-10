TOP_DIR			= ..
API_SUBDIR		= api/v1
ENV_SUBDIR		= env
ENV_REQUIREMENTS	= requirements.txt
SPEC_SUBDIRS		= spec
LOG_FILE		= openperf.log
XML_FILE		= results.xml

API_FILES = $(API_SUBDIR)/requirements.txt
API_FILES += $(shell find $(API_SUBDIR)/ -type f -name '*.py')

.PHONY: clean junit progress setup

all: progress

junit: setup
	@rm -f $(LOG_FILE) $(XML_FILE)
	$(ENV_SUBDIR)/bin/mamba $(SPEC_SUBDIRS) --format junit > $(XML_FILE)

progress: setup
	@rm -f $(LOG_FILE)
	$(ENV_SUBDIR)/bin/mamba $(SPEC_SUBDIRS)

setup: $(ENV_SUBDIR)/

$(ENV_SUBDIR)/: $(API_FILES) $(ENV_REQUIREMENTS)
	@rm -rf $(ENV_SUBDIR)
	virtualenv -p python3 $(ENV_SUBDIR)
	$(ENV_SUBDIR)/bin/pip install -r $(ENV_REQUIREMENTS)

clean:
	rm -rf $(ENV_SUBDIR)
