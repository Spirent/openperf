#!/bin/sh
# Script to map -march values to DPDK machine types.
# DPDK supports a limited subset of values.
# See contents of deps/dpdk/mk/machine.

arch=`echo "$1" | tr '[:upper:]' '[:lower:]'`

case $arch in

    sandybridge)
        echo "snb" ;;
    ivybridge)
        echo "ivb" ;;
    haswell | broadwell)
        echo "hsw" ;;
    native)
        echo "native" ;;

    *)
        echo "default" ;;

esac
