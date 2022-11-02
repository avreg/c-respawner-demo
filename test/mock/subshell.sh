#!/usr/sh

echo "$$ Subshell example"

function_to_fork() {
    echo "$$ from subshell, parent $PPID"
}

function_to_fork &