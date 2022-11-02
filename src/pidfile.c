/***
 * pidfile.c - module with helpers to create, read and remove
 *             self-app pid file:
 *               /run/respawner-{CHILD}.pid - if run as root
 *             or
 *               /tmp/respawner-{CHILD}.pid - if run as non root
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include <respawner-config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "utils/proc.h"

#include "pidfile.h"

// have no thread, may safety inner static
static char pidFileAbsName[PATH_MAX];
static int lockFd = -1;

bool respawnerPidFileCreate(const char *childBaseName, pid_t pid)
{
    uid_t me;

    assert(lockFd < 0);
    assert(pidFileAbsName[0] == '\0');

    me = geteuid();

    snprintf(pidFileAbsName, sizeof(pidFileAbsName),
             me == 0 ? "/run/" PROJECT_NAME "-%s.pid"
                     : "/tmp/" PROJECT_NAME "-%s.pid",
             childBaseName);

    lockFd = open(pidFileAbsName,
                  O_CREAT | O_EXCL | O_NOFOLLOW | O_SYNC | O_WRONLY, 0644);
    if (0 > lockFd) {
        if (errno == EEXIST) {
            lockFd = open(pidFileAbsName, O_NOFOLLOW | O_SYNC | O_WRONLY, 0644);
            if (lockFd >= 0) {
                goto doLock;
            }
        }
        syslog(LOG_ERR, "open(%s) failed -> %s", pidFileAbsName,
               strerror(errno));
        return false;
    }

doLock:
    if (0 > flock(lockFd, LOCK_EX | LOCK_NB)) {
        if (errno == EWOULDBLOCK) {
            syslog(LOG_ERR, "another respawner yet running, check pidfile %s",
                   pidFileAbsName);
        } else {
            syslog(LOG_ERR, "flock(%s) failed -> %s", pidFileAbsName,
                   strerror(errno));
        }
        close(lockFd);
        lockFd = -1;
        return false;
    }

    char buf[20];
    int buflen = snprintf(buf, sizeof(buf), "%d\n", pid);
    ssize_t wrote = write(lockFd, buf, buflen);
    if (buflen != wrote) {
        syslog(LOG_ERR, "write(%s) failed -> %s", pidFileAbsName,
               strerror(errno));
        return false;
    }

    return true;
} // respawnerPidFileCreate()

bool respawnerPidFileGet(const char *childBaseName, pid_t *pid)
{
    if (pidFileAbsName[0] == '\0') {
        uid_t me;
        me = geteuid();

        snprintf(pidFileAbsName, sizeof(pidFileAbsName),
                 me == 0 ? "/run/" PROJECT_NAME "-%s.pid"
                         : "/tmp/" PROJECT_NAME "-%s.pid",
                 childBaseName);
    }

    pid_t rpid = readPidFile(pidFileAbsName);

    if (rpid >= 0) {
        *pid = rpid;
        return true;
    } else {
        return false;
    }
} // respawnerPidFile()

bool respawnerPidFileDelete(void)
{
    int ret;

    if (lockFd < 0 || pidFileAbsName[0] == '\0') {
        return true;
    }

    close(lockFd);

    ret = unlink(pidFileAbsName);
    if (0 > ret) {
        syslog(LOG_ERR, "unlink(%s) failed -> %s", pidFileAbsName,
               strerror(errno));
        return false;
    } else {
        return true;
    }
} // pidFileDel()
