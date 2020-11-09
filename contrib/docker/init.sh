#!/usr/bin/env bash
#
# Run script for the OpenPerf docker container entrypoint
#
# This script translates container environment variables with OP_ prefix
# to the long options of the OpenPerf. All double underscores in variable
# name will be replaced by hyphens. All single underscores will be replaced
# by dots. Prefix OP_ will be stripped.
#
# Replacement rules:
#   OP_ =>
#   __  => -
#   _   => .
#
# For example:
#   OP_CONFIG => --config
#   OP_MODULES_PLUGINS_LOAD => --modules.plugins.load
#   OP_MODULES_SOCKET_FORCE__UNLINK => --modules.socket.force-unlink
#

# The executable program should be passed through EXECUTABLE
# environment variable or as first argument of this stript.
if [ -z "${EXECUTABLE}" ]; then
  EXECUTABLE=${1}
  shift
fi

# Print environment variables to console for debugging
#echo ">>> ENV Variables"
#env | sort | sed -e 's/^/   /;$a\\n'

function scan_plugins() {
  if [ ! -d "$1" ]; then
    echo "Plugins directory '${1}' doesn't exists"
    return 1
  fi

  cd "$1"
  local list=''
  for plugin in *.so; do
    list="${list},${plugin}"
  done
  echo ${list#,}
}

#OP_CONFIG=${OP_CONFIG:-'config.yaml'}
#OP_MODULES_PLUGINS_PATH=${OP_MODULES_PLUGINS_PATH:-'plugins'}
if [ -n "${OP_MODULES_PLUGINS_PATH}" ]; then
  OP_MODULES_PLUGINS_LOAD=${OP_MODULES_PLUGINS_LOAD:-$(scan_plugins ${OP_MODULES_PLUGINS_PATH})}
fi

IFS=$'\n'
OPTIONS=($(set | awk '
  BEGIN { FS = "\n"; OFS = "" };
  $1 ~ /^OP_/ {
    sub("OP_", "", $1);
    var = substr($1, 0, index($1, "="));
    val = substr($1, index($1, "=") + 1);
    gsub("__", "-", var);
    gsub("_", ".", var);

    print("--", tolower(var))
    print(val)
  }'))

#echo -e "Options: ${OPTIONS[@]}\n"

exec "${EXECUTABLE}" \
  $* "${OPTIONS[@]}"

