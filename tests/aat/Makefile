TOP_DIR			= ..
API_SUBDIR		= api/v1
ENV_SUBDIR		= env
ENV_REQUIREMENTS	= requirements.txt
SPEC_SUBDIRS		= spec

.PHONY: all setup clean

all: setup
	$(ENV_SUBDIR)/bin/mamba $(SPEC_SUBDIRS)

setup: $(ENV_SUBDIR)/

$(ENV_SUBDIR)/:
	virtualenv $(ENV_SUBDIR)
	$(ENV_SUBDIR)/bin/pip install -r $(ENV_REQUIREMENTS)

clean:
	rm -rf $(ENV_SUBDIR)
