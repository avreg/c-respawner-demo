/***
 * onsig-behaviour.c - module for signal handlers, TERM and CHLD
 *                     implement most of the respawn behavior logic
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <signal.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "conf.h"
#include "onsig-behaviour.h"

#include "utils/proc.h"

// /usr/include/asm-generic/siginfo.h: exited 1, killed 2, dumped 3, trapped 4,
// stopped 5, continued 6
#define PROC_EXITED(siginfo) ((siginfo)->ssi_code == CLD_EXITED)
#define PROC_KILLED(siginfo) ((siginfo)->ssi_code == CLD_KILLED)
#define PROC_DUMPED(siginfo) ((siginfo)->ssi_code == CLD_DUMPED)
#define PROC_TRAPPED(siginfo) ((siginfo)->ssi_code == CLD_TRAPPED)
#define PROC_STOPPED(siginfo) ((siginfo)->ssi_code == CLD_STOPPED)
#define PROC_CONTINUED(siginfo) ((siginfo)->ssi_code == CLD_CONTINUED)

#define EXITING_SUCCESSFULLY(conf, siginfo)                                    \
    ((siginfo)->ssi_code == CLD_EXITED &&                                      \
     isProcSuccessExitStatus((conf), (siginfo)->ssi_status))
#define STARTUP_FAILURE(siginfo)                                               \
    ((siginfo)->ssi_code == CLD_EXITED &&                                      \
     ((siginfo)->ssi_status == 126 || (siginfo)->ssi_status == 127))

extern enum RESPAWNER_LOOP respawnerLoop;

extern pid_t respawner_pid;
extern pid_t child_pid;
extern pid_t daemon_pid;

#define KILL_PROC(pid, procTitle)                                              \
    do {                                                                       \
        if (0 > kill(pid, SIGTERM)) {                                          \
            syslog(LOG_ERR, "kill(pid, SIGTERM)");                             \
        }                                                                      \
        syslog(LOG_INFO, "kill(%s pid %d)\n", procTitle, pid);                \
    } while (0)

bool onSelfTERM(void)
{
    // self(parent), got TERM|INT
    if (child_pid >= 0) {
        KILL_PROC(child_pid, "child");
    }
    if (daemon_pid >= 0) {
        KILL_PROC(daemon_pid, "daemon");
    }
    respawnerLoop = DO_EXIT_SUCCESS;

    return true;
} // onSelfTERM

bool onDaemonStarterCHLD(const conf_t *conf,
                         const struct signalfd_siginfo *siginfo)
{
    procParking(child_pid);
    child_pid = -1;

    if (STARTUP_FAILURE(siginfo)) {
        // file not found, permissions, etc
        respawnerLoop = DO_EXIT_FAILURE;
    } else if (EXITING_SUCCESSFULLY(conf, siginfo)) {
        daemon_pid = readPidFile(conf->child.pidfile);
        if (0 > daemon_pid) {
            respawnerLoop = DO_EXIT_FAILURE;
            return false;
        }
        // now, we need replace child_pid by daemon_pid
        if (0 > ptrace(PTRACE_SEIZE, daemon_pid, NULL, PTRACE_O_TRACEEXIT)) {
            perror("ptrace(PTRACE_SEIZE, daemon_pid)");
            respawnerLoop = DO_EXIT_FAILURE;
            return false;
        }
        syslog(LOG_INFO, "start trace %s daemon pid %d\n",
               conf->child.base_name, daemon_pid);
    }
    return true;
} // onChildCHLD()

bool onDaemonCHLD(const conf_t *conf, const struct signalfd_siginfo *siginfo)
{
    switch (siginfo->ssi_code) {
    case CLD_EXITED:
        procParking(daemon_pid);
        daemon_pid = -1;
        if (isProcSuccessExitStatus(conf, siginfo->ssi_status)) {
            respawnerLoop = DO_EXIT_SUCCESS;
            return false;
        }
        break;
    case CLD_KILLED:
    case CLD_DUMPED:
        procParking(daemon_pid);
        daemon_pid = -1;
        break;
    case CLD_TRAPPED:
        // daemon got signal
        if (0 > ptrace(PTRACE_CONT, daemon_pid, NULL, SIGTERM)) {
            perror("ptrace(PTRACE_CONT, daemon_pid)");
            respawnerLoop = DO_EXIT_FAILURE;
            return false;
        }

        break;
    default:
        // CLD_STOPPED | CLD_CONTINUED
        return true;
    }
    return true;
} // onDaemonCHLD()

bool onChildCHLD(const conf_t *conf, const struct signalfd_siginfo *siginfo)
{
    procParking(child_pid);

    child_pid = -1;

    if (STARTUP_FAILURE(siginfo)) {
        // file not found, permissions, etc
        respawnerLoop = DO_EXIT_FAILURE;
        return false;
    } else if (EXITING_SUCCESSFULLY(conf, siginfo)) {
        respawnerLoop = DO_EXIT_SUCCESS;
    }

    return true;
} // onChildCHLD()
