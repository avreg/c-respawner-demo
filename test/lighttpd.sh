#!/bin/sh

sudo -E ./build/src/respawner \
    -e 0,1 -p start \
    lighttpd -- -f /etc/lighttpd/lighttpd.conf
