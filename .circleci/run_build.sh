set -xe

# If we build too many files in parallel, we'll hit the CircleCI
# container's memory limit. Hence, we artificially limit the number
# of parallel builds here.
MAX_CORES=12

CI_CORES=$(if [ $(nproc) -gt ${MAX_CORES} ]; then echo ${MAX_CORES}; else echo $(nproc); fi)

# Force optimizations to target avx2; all of the CircleCI machines seem
# to support it and we don't want to get illegal instruction errors when
# we hand these objects off to the acceptance test job.
# Some of those build servers are fancy and support AVX512 which the default
# compile options will use, if available.
CI_BUILD_OPTS='-Os -gline-tables-only -march=core-avx2'

# Use bear to generate the compilation database.
# Treat first CLI argument to this script as additional parameter(s) to make.
bear make -j${CI_CORES} OP_COPTS="${CI_BUILD_OPTS}" $1 all
