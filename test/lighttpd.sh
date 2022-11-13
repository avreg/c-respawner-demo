#!/bin/sh

sudo -E ./build/src/c-respawner-demo \
    -e 0,1 -p start \
    lighttpd -- -f /etc/lighttpd/lighttpd.conf
