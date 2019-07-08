TOP_DIR			= ..
API_SUBDIR		= api/v1
ENV_SUBDIR		= env
ENV_REQUIREMENTS	= requirements.txt
SPEC_SUBDIRS		= spec
LOG_FILE		    = inception.log

.PHONY: all setup clean

all: setup
	-@rm -f $(LOG_FILE)
	$(ENV_SUBDIR)/bin/mamba $(SPEC_SUBDIRS)

setup: $(ENV_SUBDIR)/

$(ENV_SUBDIR)/:
	virtualenv $(ENV_SUBDIR)
	$(ENV_SUBDIR)/bin/pip install -r $(ENV_REQUIREMENTS)

clean:
	rm -rf $(ENV_SUBDIR)
