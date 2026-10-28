#!/bin/bash

# Copyright 2020 Rene Rivera, Sam Darwin
# Copyright 2022 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

set -ex
source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"
./b2 -q -j3 --user-config="$B2_USER_CONFIG" "libs/$LIBRARY/test" toolset="$TOOLSET" cxxstd="$CXXSTD" variant=debug,release ${ADDRMD:+address-model=$ADDRMD} ${UBSAN:+undefined-sanitizer=norecover debug-symbols=on} ${ASAN:+address-sanitizer=norecover debug-symbols=on} ${LINKFLAGS:+linkflags=$LINKFLAGS} ${LINK:+link=$LINK} ${NO_ZSTD:+-sNO_ZSTD=$NO_ZSTD}
