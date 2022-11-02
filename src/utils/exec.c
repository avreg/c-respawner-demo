/***
 * utils/exec.c - exec utility helpers
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include <respawner-config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>

#include "exec.h"

pid_t execProc(const char *pathname, char *const argv[])
{
    pid_t child_pid;

    child_pid = fork();
    if (child_pid < 0) {
        syslog(LOG_PERROR, "fork()");
    } else if (child_pid == 0) {
        // child

        /* Close all open by parent respawner proc file descriptors */
        for (int x = sysconf(_SC_OPEN_MAX); x >= STDERR_FILENO; x--) {
            close(x);
        }

        // unblock set of blocked signals
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGQUIT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        execvp(pathname, argv);
        syslog(LOG_PERROR, "execvp()");
        if (errno == ENOENT) {
            exit(127); // command not found
        } else {
            exit(126); // not an executable(?)
        }
    }

    return child_pid;
} // execProc()
