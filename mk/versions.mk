# Set up various variables to update build info at build time.
# Values include:
#   GIT_COMMIT - latest version control commit.
#   GIT_VERSION - latest version control tag, aka version
#   BUILD_NUMBER - build number from CI environment. 0 if not set.
#   TIMESTAMP - Current time in format set by organization.

# Tag the build with the git commit
GIT_COMMIT := $(shell git rev-parse --verify HEAD)
# If working tree is dirty, append dirty flag
ifneq ($(strip $(shell git status --porcelain 2>/dev/null)),)
	GIT_COMMIT := $(GIT_COMMIT)-dirty
endif

GIT_VERSION := $(shell git describe --always --tags)

# Jenkins sets BUILD_NUMBER explicitly.  If we're running under CirclCI,
# then use that, otherwise just use the git hash as the build number
ifeq ($(BUILD_NUMBER),)
	ifneq ($(CIRCLE_BUILD_NUM),)
		BUILD_NUMBER = $(CIRCLE_BUILD_NUM)
	else
		BUILD_NUMBER = $(shell git rev-parse --short HEAD)
  endif
endif

#Generate current timestamp per organizational requirements (ISO 8601).
#This is the portable version of GNU date's --iso-8601 option.
TIMESTAMP := $(shell date +%Y-%m-%dT%H:%M:%S%z | sed 's@^.\{22\}@&:@')

# This dummy target allows us to treat versions as a standard dependency
.PHONY: versions
