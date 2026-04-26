set -xe

SDK_SYSROOT="/usr/aarch64-linux-gnu"
SDK_CPP_VERSION=$(aarch64-linux-gnu-gcc -dumpversion)
SDK_CPPFLAGS="-nostdinc++ -cxx-isystem ${SDK_SYSROOT}/include/c++/${SDK_CPP_VERSION}  -cxx-isystem ${SDK_SYSROOT}/include/c++/${SDK_CPP_VERSION}/aarch64-linux-gnu"
SDK_CROSS="aarch64-linux-gnu-"
SDK_LDFLAGS=""

ARGS=${@:-""}

# If we build too many files in parallel, we'll hit the CircleCI
# container's memory limit. Hence, we artificially limit the number
# of parallel builds here.
MAX_CORES=12

CI_CORES=$(if [ $(nproc) -gt ${MAX_CORES} ]; then echo ${MAX_CORES}; else echo $(nproc); fi)

# Note: Need to use / for SYSROOT here because of how debian cross packages work
make ARCH=aarch64 \
    OP_SYSROOT="/" \
    OP_EXTRA_CPPFLAGS="${SDK_CPPFLAGS}" \
    OP_DPDK_CROSS="${SDK_CROSS}" \
    OP_EXTRA_LDFLAGS="${SDK_LDFLAGS}" \
    -j${CI_CORES} ${ARGS}
