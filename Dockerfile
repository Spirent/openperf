FROM debian:buster

# Inform apt-get that it'll get no such satisfaction from us
ENV DEBIAN_FRONTEND noninteractive

###
# Install all of our dependencies
###

# Install LLVM and various other tooling
ENV LLVM_VERSION 7
RUN apt-get clean && \
    apt-get update && \
    apt-get install -y --no-install-recommends autoconf automake \
    build-essential ca-certificates git libcap-dev libcap2-bin libnuma-dev \
    libtool netcat-openbsd pkg-config python sudo virtualenv wget \
    clang-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} \
    llvm-${LLVM_VERSION}-dev

# Fix up toolchain names
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 10 \
                        --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} && \
    update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-${LLVM_VERSION} 10 && \
    update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 10

# Install other Circle CI dependencies
RUN apt-get install -y --no-install-recommends openssh-client
