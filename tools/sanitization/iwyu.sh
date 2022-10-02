#!/bin/bash

# check include what you use

echo "NOTE: to automagically apply fixes, invoke with --fix"

set -ex

cd $(dirname $0)/../..
REPO_ROOT=$(pwd)

tools/sanitization/gen_compilation_database.py \
  --include_headers \
  --ignore_system_headers

# run iwyu against the checked out codebase
# when modifying the checked-out files, the current user will be impersonated
# so that the updated files don't end up being owned by "root".
if [ "$IWYU_SKIP_DOCKER" == "" ]
then
  # build iwyu docker image
  docker build -t mb_iwyu tools/dockerfile/mb_iwyu

  docker run \
    -e IWYU_ROOT="/local-code" \
    -e REPO_ROOT="${REPO_ROOT}" \
    --rm=true \
    -v "${REPO_ROOT}":/local-code \
    --user "$(id -u):$(id -g)" \
    -t mb_iwyu /iwyu.sh "$@"
else
  IWYU_ROOT="${REPO_ROOT}" tools/dockerfile/mb_iwyu/iwyu.sh
fi