#!/bin/bash

printf "\n***Starting Service***\n"

check_command="/sbin/setcap -q -v CAP_IPC_LOCK,CAP_NET_RAW,CAP_SYS_ADMIN,CAP_DAC_OVERRIDE,CAP_SYS_PTRACE=epi $1"

if $check_command; then
    exec "$@"
else
    exec /usr/bin/sudo "$@"
fi
