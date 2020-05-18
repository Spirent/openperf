set -xe

# The container apparently sees the correct amount of memory, but not
# not the correct number of cores.  If we use more than ~6 cores, we
# seem to hit the memory limit and our build commands can get killed.
CI_CORES=$(if [ $(nproc) -gt 6 ]; then echo 6; else echo $(nrpco); fi)

make -j${CI_CORES} test_unit
