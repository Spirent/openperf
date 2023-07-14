#!/bin/bash
#
# Create Dockerfile from CI Dockerfile for the devcontainer
#

DEVCONTAINER_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
TOP_DIR=$(realpath ${DEVCONTAINER_DIR}/..)
BUILD_DIR=${TOP_DIR}/build
DOCKERFILE="Dockerfile"

mkdir -p ${BUILD_DIR}

cp -f ${TOP_DIR}/${DOCKERFILE} ${BUILD_DIR}/${DOCKERFILE}

cat << EOF >> ${BUILD_DIR}/${DOCKERFILE}


# Install Java for code generator
RUN apt-get update && apt-get install -y --no-install-recommends default-jre-headless

# Install golang for code generator
ENV GO_VERSION=1.16.5
RUN wget https://dl.google.com/go/go\${GO_VERSION}.linux-amd64.tar.gz \\
    && tar zxf go\${GO_VERSION}.linux-amd64.tar.gz -C /usr/local \\
    && rm -f go\${GO_VERSION}.linux-amd64.tar.gz \\
    && update-alternatives --install /usr/bin/go go /usr/local/go/bin/go 1 \\
    && update-alternatives --install /usr/bin/gofmt gofmt /usr/local/go/bin/gofmt 1

# Install gdb
RUN apt-get update && apt-get install -y gdb


# Add user account w/ sudo privileges
# Note:
#   Current vscode remote development package v0.24.0 has an issue when initializing
#   the ~/.ssh directory.  The issue does not occur when mounting the host ~/.ssh
#   directory or if a ~/.ssh/known_hosts file exists.
RUN useradd ${USER} --home-dir=/home/${USER} --create-home --shell=/bin/bash \\
    && echo "${USER} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${USER} \\
    && mkdir -p /home/${USER}/.ssh \\
    && touch /home/${USER}/.ssh/known_hosts \\
    && chown -R ${USER} /home/${USER}

EOF
