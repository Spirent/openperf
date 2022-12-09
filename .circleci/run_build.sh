set -xe

# If we build too many files in parallel, we'll hit the CircleCI
# container's memory limit. Hence, we artificially limit the number
# of parallel builds here.
MAX_CORES=12

CI_CORES=$(if [ $(nproc) -gt ${MAX_CORES} ]; then echo ${MAX_CORES}; else echo $(nproc); fi)

# Force optimizations to target avx2; all of the CircleCI machines seem
# to support it and we don't want to get illegal instruction errors when
# we hand these objects off to the acceptance test job.
CI_ARCH=core-avx2

# Use bear to generate the compilation database.
# Treat first CLI argument to this script as additional parameter(s) to make.
bear make -j${CI_CORES} OP_MACHINE_ARCH=${CI_ARCH} $1 all
