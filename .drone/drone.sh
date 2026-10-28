#!/bin/bash

# Copyright 2020 Rene Rivera, Sam Darwin
# Copyright 2022 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

set -ex
export PATH=~/.local/bin:/usr/local/bin:$PATH

DRONE_BUILD_DIR=$(pwd)

BOOST_BRANCH=develop
if [ "$DRONE_BRANCH" = "master" ]; then BOOST_BRANCH=master; fi

cd ..
git clone -b "$BOOST_BRANCH" --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root
git submodule update --init tools/boostdep
cp -R "$DRONE_BUILD_DIR/"* "libs/$LIBRARY/"
python tools/boostdep/depinst/depinst.py "$LIBRARY"
./bootstrap.sh
./b2 -d0 headers

# Build these from the bundled sources for every target architecture.
export ZLIB_SOURCE="$PWD/libs/$LIBRARY/test/zlib-1.2.11"
export BZIP2_SOURCE="$PWD/libs/$LIBRARY/test/bzip2-1.0.8"

echo "using $TOOLSET : : $COMPILER ;" > ~/user-config.jam
./b2 -j3 "libs/$LIBRARY/test" toolset="$TOOLSET" cxxstd="$CXXSTD" variant=debug,release ${ADDRMD:+address-model=$ADDRMD} ${UBSAN:+undefined-sanitizer=norecover debug-symbols=on} ${ASAN:+address-sanitizer=norecover debug-symbols=on} ${LINKFLAGS:+linkflags=$LINKFLAGS} ${LINK:+link=$LINK} ${NO_ZSTD:+-sNO_ZSTD=$NO_ZSTD}
