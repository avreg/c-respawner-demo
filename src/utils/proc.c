/***
 * utils/proc.c - process-level utility helpers
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include <respawner-config.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>

#include "proc.h"

enum PROC_STATES parse_wstatus(int wstatus, int *status, int *signo)
{
    if (WIFEXITED(wstatus)) {
        *status = WEXITSTATUS(wstatus);
        return PROC_TERMINATED;
    } else if (WIFSIGNALED(wstatus)) {
        *signo = WTERMSIG(wstatus);
        return PROC_SIGNALED;
    } else if (WIFSTOPPED(wstatus)) {
        *signo = WSTOPSIG(wstatus);
        return PROC_STOPPED;
    } else if (WIFCONTINUED(wstatus)) {
        *signo = SIGCONT;
        return PROC_STARTED;
    } else {
        return PROC_UNKNOWN;
    }
} // parse_wstatus()

pid_t readPidFile(const char *pidfilepath)
{
    FILE *f;
    pid_t pid = -1;

    if (NULL != (f = fopen(pidfilepath, "r"))) {
        if (1 != fscanf(f, "%d", &pid)) {
            syslog(LOG_ERR, "couldn't read pid from \"%s\" pidfile\n", pidfilepath);
        }
        fclose(f);
    } else {
        syslog(LOG_ERR, "couldn't open \"%s\" pidfile -> %s\n", pidfilepath,
               strerror(errno));
    }
    return pid;
} // readPidFile()

// prevent zombie
void procParking(pid_t pid)
{
    int wstatus, status, signo;
    enum PROC_STATES ps;

    if (0 > waitpid(pid, &wstatus, 0)) {
        // unrecoverable parent error
        perror("waitpid(pid) after SIGCHLD");
        return;
    }

    ps = parse_wstatus(wstatus, &status, &signo);
    if (ps == PROC_TERMINATED) {
        syslog(LOG_NOTICE, "proc %d exited, status %d", pid, status);
    } else {
        syslog(LOG_NOTICE, "proc %d %s by signo %d\n", pid,
               ps == PROC_SIGNALED  ? "killed"
               : ps == PROC_STOPPED ? "stopped"
                                    : "started",
               signo);
    }
} // procParking()
