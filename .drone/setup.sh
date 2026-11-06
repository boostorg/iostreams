#!/bin/bash

# Copyright 2022 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

# Source this from a Drone build script; it leaves the shell in boost-root.
export PATH=~/.local/bin:/usr/local/bin:$PATH
export DRONE_BUILD_DIR="$PWD"

BOOST_BRANCH=develop
if [ "${DRONE_BRANCH:-}" = "master" ]; then BOOST_BRANCH=master; fi

cd ..
git clone -b "$BOOST_BRANCH" --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root
export BOOST_ROOT="$PWD"
git submodule update --init tools/boostdep
cp -R "$DRONE_BUILD_DIR/"* "libs/$LIBRARY/"
python3 tools/boostdep/depinst/depinst.py "$LIBRARY"
./bootstrap.sh
./b2 -d0 headers

# Linux jobs use the compression development packages installed by the pipeline.
unset ZLIB_SOURCE BZIP2_SOURCE

export B2_USER_CONFIG="$DRONE_BUILD_DIR/user-config.jam"
echo "using $TOOLSET : : $COMPILER ;" > "$B2_USER_CONFIG"
