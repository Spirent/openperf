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
RUN wget https://dl.google.com/go/go\${GO_VERSION}.linux-amd64.tar.gz && \\
    tar zxf go\${GO_VERSION}.linux-amd64.tar.gz -C /usr/local && \\
    rm -f go\${GO_VERSION}.linux-amd64.tar.gz && \\
    update-alternatives --install /usr/bin/go go /usr/local/go/bin/go 1 && \\
    update-alternatives --install /usr/bin/gofmt gofmt /usr/local/go/bin/gofmt 1

# Install gdb
RUN apt-get update && apt-get install -y gdb

# Add user account w/ sudo privileges
RUN useradd ${USER} --home-dir=/home/${USER} --create-home --shell=/bin/bash && \\
    echo "${USER} ALL=(root) NOPASSWD: ALL" >> /etc/sudoers.d/${USER}

EOF
