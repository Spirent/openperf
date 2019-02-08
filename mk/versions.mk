# Set up various variables to update build info at build time.
# Values include:
#   GIT_COMMIT - latest version control commit.
#   BUILD_NUMBER - build number from Jenkins. 0 if not set.
#   TIMESTAMP - Current time in format set by organization.

# Tag the build with the git commit version
GIT_COMMIT := $(shell git rev-parse --verify HEAD)
# If working tree is dirty, append dirty flag
ifneq ($(strip $(shell git status --porcelain 2>/dev/null)),)
	GIT_COMMIT := $(GIT_COMMIT)-dirty
endif

# Use the Jenkins build number, if present, to tag the build.
ifeq ($(BUILD_NUMBER),)
BUILD_NUMBER = 0
$(warning No BUILD_NUMBER defined in environment, using 0)
endif

#Generate current timestamp per organizational requirements.
TIMESTAMP := $(shell date -u +%Y-%m-%dT%H:%M:%S%Z)
