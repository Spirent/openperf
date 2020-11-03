#!/usr/bin/env bash

function clean_folder() {
    rm -rf "$ROOT/build/docker"
}

cd "$(dirname "$0")" || exit 1
ROOT=$(pwd)/../..

trap 'clean_folder' ERR
set -e

clean_folder
mkdir -p "$ROOT/build/docker"
cp -r "$ROOT/.circleci" \
      "$ROOT/conf" \
      "$ROOT/deps" \
      "$ROOT/mk" \
      "$ROOT/src" \
      "$ROOT/targets" \
      "config.yaml" \
      "init.sh" \
      "$ROOT/build/docker/"

docker build \
    --file Dockerfile \
    --tag "${DOCKER_IMAGE}" \
    --build-arg GIT_COMMIT="${GIT_COMMIT}" \
    --build-arg GIT_VERSION="${GIT_VERSION}" \
    --build-arg BUILD_NUMBER="${BUILD_NUMBER}" \
    "$ROOT/build/docker"

clean_folder
