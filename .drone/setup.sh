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

# Build these from the bundled sources for every target architecture.
export ZLIB_SOURCE="$BOOST_ROOT/libs/$LIBRARY/test/zlib-1.2.11"
export BZIP2_SOURCE="$BOOST_ROOT/libs/$LIBRARY/test/bzip2-1.0.8"

export B2_USER_CONFIG="$DRONE_BUILD_DIR/user-config.jam"
echo "using $TOOLSET : : $COMPILER ;" > "$B2_USER_CONFIG"
