set -e

TIDY_OUTPUT=$(mktemp)
EXIT_CODE=0

# Limit how many tidy jobs the container can run in parallel. CircleCi
# containers have too little memory to spin up one process per core.
MEM_JOBS=6
CPU_JOBS=$(nproc)
if [[ ${MEM_JOBS} -lt ${CPU_JOBS} ]]; then
    export TIDY_JOBS=${MEM_JOBS}
else
    export TIDY_JOBS=${CPU_JOBS}
fi

# CirclCi needs constant feedback that it's doing something useful. Any
# long running command without output could cause the job to fail, so
# turn the tidy output into a concise series of periods; one per file.
# We also write the full log to a temp file so we can parse it for
# legitimate errors.
# Note: the circleci/docker container needs output explicitly set to
# unbuffered, otherwise no periods will be printed until the command
# finishes. This is unnecessary when run from a regular terminal,
# however.
echo -n "Analyzing files (TIDY_JOBS=${TIDY_JOBS})"
make tidy 2> /dev/null | tee ${TIDY_OUTPUT} | grep 'clang-tidy' | stdbuf -o0 awk '{printf "."}'
echo

if [[ -n $(grep "error: " ${TIDY_OUTPUT}) ]]; then
    echo "clang-tidy errors detected; please fix them:"
    grep -v -E '(^$|clang-tidy)' ${TIDY_OUTPUT} | grep --color -E '|error: '
    EXIT_CODE=1
fi

# Unfortunately, there appears to be no way to get clang-tidy to skip
# headers with inline functions and DPDK's rte_ip.h header contains a
# false positive.  Hence, ignore any warnings for it.
DPDK_WARN_FILTER="dpdk/include/rte_ip.h"

if [[ -n $(grep "warning: " ${TIDY_OUTPUT} | grep -v ${DPDK_WARN_FILTER}) ]]; then
    echo "Non-DPDK clang-tidy warnings detected; please fix them:"
    grep -v -E '(^$|clang-tidy)' ${TIDY_OUTPUT} | grep --color -E '|warning: '
    EXIT_CODE=1
fi

rm ${TIDY_OUTPUT}
exit ${EXIT_CODE}
