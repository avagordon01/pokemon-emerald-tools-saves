#!/usr/bin/env bash

set -ex

if [ ! -d builddir ]; then
    meson setup builddir
fi
pushd builddir
meson compile
popd
