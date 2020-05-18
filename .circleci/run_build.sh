set -xe

# The container apparently sees the correct amount of memory, but not
# not the correct number of cores.  If we use more than ~6 cores, we
# seem to hit the memory limit and our build commands can get killed.
CI_CORES=$(if [ $(nproc) -gt 6 ]; then echo 6; else echo $(nrpco); fi)

# Force optimizations to target avx2; all of the CircleCI machines seem
# to support it and we don't want to get illegal instruction errors when
# we hand these objects off to the acceptance test job.
# Some of those build servers are fancy and support AVX512 which the default
# compile options will use, if available.
CI_BUILD_OPTS='-Os -gline-tables-only -march=core-avx2'

# Use bear to generate the compilation database
bear make -j${CI_CORES} OP_COPTS="${CI_BUILD_OPTS}" all
