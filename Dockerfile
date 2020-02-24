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
    clang-${LLVM_VERSION} clang-format-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} \
    llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev \
    lld-${LLVM_VERSION} bear

# Install Intel SPMD Program Compiler (ISPC)
ENV ISPC_VERSION 1.12.0
RUN wget -q -O - https://sourceforge.net/projects/ispcmirror/files/v${ISPC_VERSION}/ispc-v${ISPC_VERSION}-linux.tar.gz \
    | tar -C /opt -xvz && \
    ln -s /opt/ispc-v${ISPC_VERSION}-linux/bin/ispc /usr/local/bin/ispc

# Fix up toolchain names
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} ${LLVM_VERSION} \
                        --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${LLVM_VERSION} ${LLVM_VERSION} && \
    update-alternatives --install /usr/bin/run-clang-tidy run-clang-tidy /usr/bin/run-clang-tidy-${LLVM_VERSION} ${LLVM_VERSION} && \
    update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-${LLVM_VERSION} ${LLVM_VERSION} && \
    update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} ${LLVM_VERSION}

# Install other Circle CI dependencies
RUN apt-get install -y --no-install-recommends openssh-client
