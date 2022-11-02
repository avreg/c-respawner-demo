#!/bin/sh

success_exiting() {
    echo "$$ Got TERM sig, success exiting with zero status"
    exit 0
}

trap success_exiting TERM

echo "$$ Forever loop"

while true; do
    sleep 1
done
