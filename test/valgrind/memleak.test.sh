#!/bin/sh

set -e

ADDONS_OPT=
if [ -s "/usr/lib/valgrind/debian.supp" ]; then
    ADDONS_OPT="${ADDONS_OPT} --suppressions=/usr/lib/valgrind/debian.supp"
fi

valgrind \
    --tool=memcheck \
    --trace-children=no \
    --track-origins=yes \
    --leak-check=full \
    --leak-resolution=high --num-callers=40 \
    --read-var-info=yes \
    --show-reachable=no \
    --show-below-main=yes \
    $ADDONS_OPT \
    build/src/respawner -D start test/mock/forever.sh
