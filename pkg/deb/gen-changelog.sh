#!/bin/bash
# Writes to STDOUT a deb package changelog file based on the git tags following a release
# version tag format. There are some limitations to the script. It is expected that each release
# will have at least one commit (without this, the changelog format will not be accepted by the
# packaging tools). Furthermore, the script does not report the very first commit due to how the
# git log query ranges work (i.e. omitting the last entry).
#
# The script takes an optional package version. If not supplied, it is defaulted to 1.
PKG_VERSION="${1:-1}"

# Get the release tags and their dates (we get
# the tags in reverse order - newest to oldest)
OIFS="${IFS}"; IFS=$'\n';
# shellcheck disable=SC2207
TAGS=($(git --no-pager for-each-ref --sort=creatordate \
    --format '%(refname:strip=2)|%(creatordate)' refs/tags | \
    grep -E '^v[0-9]+\.[0-9]+\.[0-9]+\|' | tac))
IFS="${OIFS}"

# Get the hash for the oldest comment (we put a fake date on
# the end to make the hash look like one of the release tags)
TAGS+=("$(git log --reverse | head -n 1 | cut -d' ' -f 2)|DATE_NOT_USED")

# Create a changelog section for each release tag
CURR_INDEX=0
while : ; do
    # Going from newest tags to older tags, get the previous tag (this exits the
    # loop if we find there is no previous tag and hence at the end of the sections)
    PREV_INDEX=$((CURR_INDEX+1))
    PREV="${TAGS[${PREV_INDEX}]}"
    [[ -n "${PREV}" ]] || break
    PREV_TAG="${PREV%%|*}"

    # Get the current release tag
    CURR="${TAGS[$CURR_INDEX]}"
    CURR_TAG="${CURR%%|*}"
    CURR_DATE="${CURR#*|}"
    CURR_DATE="${CURR_DATE%% *}"
    CURR_DATE="$(date -d "${CURR_DATE}" +"%a, %d %b %Y %X %z")"

    # Create the section of output for the current release tag
    echo "openperf (${CURR_TAG#v}-${PKG_VERSION}) unstable; urgency=low";
    echo
    git --no-pager log --pretty=format:'  * %s' "${PREV_TAG}".."${CURR_TAG}"
    echo
    echo
    echo " -- Spirent Builder <support@spirent.com>  ${CURR_DATE}"
    echo

    # Setup to process the next section
    CURR_INDEX="${PREV_INDEX}"
done
exit 0
