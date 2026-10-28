#!/bin/bash

# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

set -euo pipefail

case "${1:-build}" in
build)
    source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"
    ./b2 -q -j3 --user-config="$B2_USER_CONFIG" "libs/$LIBRARY/test" \
        toolset="$TOOLSET" cxxstd="$CXXSTD" address-model=64 link=static \
        variant=debug optimization=off debug-symbols=on coverage=on

    python3 -m venv "$DRONE_BUILD_DIR/.coverage-venv"
    "$DRONE_BUILD_DIR/.coverage-venv/bin/pip" install --disable-pip-version-check \
        gcovr==8.8 codecov-cli==11.3.1

    # gcovr resolves the Boost header symlinks before applying source filters.
    cd "$BOOST_ROOT/libs/$LIBRARY"
    "$DRONE_BUILD_DIR/.coverage-venv/bin/gcovr" "$BOOST_ROOT" \
        --root . --filter 'include/' --filter 'src/' --gcov-executable "$GCOV" \
        --merge-mode-functions merge-use-line-min --print-summary \
        --lcov-format-version 1.x --lcov "$DRONE_BUILD_DIR/coverage.info"
    # LCOV stores absolute paths; map the Boost checkout back to this repository.
    python3 - "$DRONE_BUILD_DIR/coverage.info" <<'PY'
from pathlib import Path
import sys

root = Path.cwd().resolve()
report = Path(sys.argv[1])
lines = report.read_text(encoding="utf-8").splitlines(keepends=True)
for index, line in enumerate(lines):
    if line.startswith("SF:"):
        source = Path(line[3:].strip()).resolve().relative_to(root)
        lines[index] = "SF:" + source.as_posix() + "\n"
report.write_text("".join(lines), encoding="utf-8")
PY
    # Do not silently publish an empty report if coverage collection fails.
    grep -q '^SF:' "$DRONE_BUILD_DIR/coverage.info"
    ;;
upload)
    if [ -z "${CODECOV_TOKEN:-}" ]; then
        echo "CODECOV_TOKEN is unavailable; coverage.info was generated, upload skipped."
        exit 0
    fi

    args=(upload-process --fail-on-error --disable-search --plugin noop
        --file "$PWD/coverage.info" --network-root-folder "$PWD"
        --git-service github --slug "${DRONE_REPO:?}"
        --sha "${DRONE_COMMIT_SHA:-${DRONE_COMMIT:?}}"
        --branch "${DRONE_SOURCE_BRANCH:-${DRONE_BRANCH:?}}"
        --name drone-linux-gcc --flag linux)
    if [ -n "${DRONE_PULL_REQUEST:-}" ]; then
        args+=(--pr "$DRONE_PULL_REQUEST")
    fi
    if [ -n "${DRONE_BUILD_LINK:-}" ]; then
        args+=(--build-url "$DRONE_BUILD_LINK")
    fi
    # The CLI reads CODECOV_TOKEN from the environment; keep it out of arguments.
    .coverage-venv/bin/codecovcli "${args[@]}"
    ;;
*)
    echo "Usage: $0 [build|upload]" >&2
    exit 1
    ;;
esac
