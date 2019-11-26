set -e

TIDY_OUTPUT=$(mktemp)
EXIT_CODE=0

make tidy > ${TIDY_OUTPUT} 2> /dev/null

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
