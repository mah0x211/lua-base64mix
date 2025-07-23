#!/usr/bin/env sh

set -ex

LCOV_IGNORE_ERRORS="--ignore-errors unused"

mkdir -p ./coverage
lcov -c -d ./src -o coverage/lcov.info.all
lcov -r coverage/lcov.info.all \
    '*/include/*' \
    '*/base64mix.h' \
    $LCOV_IGNORE_ERRORS \
    -o coverage/lcov.info
# genhtml -o coverage/html coverage/lcov.info
