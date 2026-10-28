#!/bin/bash

# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

set -euo pipefail

# Keep the script safe to call directly as well as through the pipeline trigger.
if [ "${DRONE_BUILD_EVENT:-}" != "push" ] || \
    { [ "${DRONE_BRANCH:-}" != "develop" ] && [ "${DRONE_BRANCH:-}" != "master" ] && \
      [ "${DRONE_BRANCH:-}" != "fix/drone-ci" ]; }; then
    echo "Coverity submissions run only on pushes to develop, master or fix/drone-ci."
    exit 0
fi
if [ -z "${COVERITY_SCAN_TOKEN:-}" ] || [ -z "${COVERITY_SCAN_NOTIFICATION_EMAIL:-}" ]; then
    echo "Coverity credentials are unavailable; scan skipped."
    exit 0
fi

source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"
project="${COVERITY_SCAN_PROJECT:-${DRONE_REPO:?}}"
tool_dir="$DRONE_BUILD_DIR/.coverity-tool"
capture_dir="$DRONE_BUILD_DIR/cov-int"
mkdir -p "$tool_dir"
curl --fail --silent --show-error --location --retry 5 \
    --data-urlencode "token=$COVERITY_SCAN_TOKEN" --data-urlencode "project=$project" \
    https://scan.coverity.com/download/cxx/linux64 -o "$tool_dir/tool.tgz"
tar -xzf "$tool_dir/tool.tgz" -C "$tool_dir" --strip-components=1

# Capture the library and test translation units to instantiate header templates.
"$tool_dir/bin/cov-build" --dir "$capture_dir" \
    ./b2 -a -j3 --user-config="$B2_USER_CONFIG" "libs/$LIBRARY/test" \
    toolset="$TOOLSET" cxxstd="$CXXSTD" address-model=64 link=static variant=debug
tar -cJf "$DRONE_BUILD_DIR/cov-int.tar.xz" -C "$DRONE_BUILD_DIR" cov-int

# Avoid shell tracing here: the token and notification email are Drone secrets.
project_query=$(python3 -c 'import sys, urllib.parse; print(urllib.parse.quote(sys.argv[1], safe=""))' "$project")
curl --fail --silent --show-error \
    --form-string "token=$COVERITY_SCAN_TOKEN" \
    --form-string "email=$COVERITY_SCAN_NOTIFICATION_EMAIL" \
    --form "file=@$DRONE_BUILD_DIR/cov-int.tar.xz" \
    --form-string "version=${DRONE_COMMIT_SHA:-${DRONE_COMMIT:?}}" \
    --form-string "description=$DRONE_REPO ($DRONE_BRANCH)" \
    "https://scan.coverity.com/builds?project=$project_query"
