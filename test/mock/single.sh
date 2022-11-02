#!/bin/sh

EXIT_CODE=${1:-0}
DELAY=${2:-2}

success_exiting() {
    echo "$$ Got TERM sig, success exiting with zero status"
    exit 0
}

trap success_exiting TERM

echo "$$ Hello, wait ${DELAY} sec"

sleep ${DELAY}

echo "$$ Bye, exit code ${EXIT_CODE}"

exit ${EXIT_CODE}
