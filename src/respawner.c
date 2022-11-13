/***
 * respawner.c - main module of respawner app
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
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/ptrace.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "conf.h"
#include "pidfile.h"

#include "utils/exec.h"
#include "utils/proc.h"
#include "utils/signal.h"
#include "utils/timer.h"

#include "onsig-behaviour.h"

#define MAX_EVENTS 10

// clang-format off
static conf_t conf = {
    .daemonize = true,
    .comm = COMM_INVAL,
    .respawn = {
        .max = 0, ///<no limit
        .timeout = 5
    }
};
// clang-format on

#define DEALING_WITH_DAEMON(conf)                                              \
    ((conf)->child.pidfile != NULL && (conf)->child.pidfile[0] != '\0')

#define IS_CONTINUE_RESPAWN                                                    \
    (respawnerLoop == CONTINUE_RESPAWN || child_pid >= 0 || daemon_pid >= 0)

/// Флаг для выхода из основного рабочего цикла, обеспечивающего respawn
enum RESPAWNER_LOOP respawnerLoop = CONTINUE_RESPAWN;

pid_t respawner_pid = -1; /// собственный pid
pid_t child_pid = -1; /// pid потомка или породителя демона
pid_t daemon_pid = -1; /// pid демона

/**
 * @brief Обработчик полученного сигнала.
 *
 * Источником могут быть внешние процессы (завершение работы),
 * процесс-потомок (не демон),
 * процесс-потомок (родитель демона),
 * трассируемый процесс демон.
 *
 * @param siginfo контекст сигнала
 * @return true продолжить разбор сигналов (если несколько)
 * @return false прекратить разбор сигналов (если несколько)
 */
static bool signal_callback(const struct signalfd_siginfo *siginfo)
{
#if 0
    syslog(LOG_INFO,
           "signal_callback(pid %d, signo %d, sigcode %d, status %d\n",
           siginfo->ssi_pid, siginfo->ssi_signo, siginfo->ssi_code,
           siginfo->ssi_status);
#endif

    if (siginfo->ssi_pid != (uint32_t)daemon_pid &&
        siginfo->ssi_pid != (uint32_t)child_pid) {
        // self(parent), got TERM|INT
        return onSelfTERM();
    }

    if (siginfo->ssi_signo == SIGCHLD) {
        if (DEALING_WITH_DAEMON(&conf)) {
            // respawner daemon behaviour:
            //      --execve()--> child --fork()--> daemon
            if (siginfo->ssi_pid == (uint32_t)child_pid) {
                return onDaemonStarterCHLD(&conf, siginfo);
            } else if (siginfo->ssi_pid == (uint32_t)daemon_pid) {
                return onDaemonCHLD(&conf, siginfo);
            }
        } else {
            // respawner single behaviour: --execve()--> child (no daemon)
            if (siginfo->ssi_pid == (uint32_t)child_pid) {
                return onChildCHLD(&conf, siginfo);
            }
        }
    }
    return IS_CONTINUE_RESPAWN;
} // signal_callback()

int main(int argc, char *argv[] /*,  char *envp[] */)
{
    int self_exit_status = EXIT_FAILURE;

    struct epoll_event events[MAX_EVENTS];
    int timer_fdes = -1, signal_fdes = -1, epoll_fdes = -1;
    int n, nfds;
    unsigned int respawnNbr = 0;
    int ret;

    // Init Init Init
    if (!getConf(argc, argv, &conf)) {
        goto laFinite;
    }

    openlog(PROJECT_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR, LOG_DAEMON);

    // отрабатываем stop|status команды и выходим
    if (conf.comm == COMM_STATUS || conf.comm == COMM_STOP) {
        if (!respawnerPidFileGet(conf.child.base_name, &respawner_pid)) {
            goto status_stop_exiting;
        }

        if (conf.comm == COMM_STOP) {
            if (0 > kill(respawner_pid, SIGTERM)) {
                perror("kill(respawner_pid, SIGTERM)");
                goto status_stop_exiting;
            }
            self_exit_status = EXIT_SUCCESS;
        }
        if (conf.comm == COMM_STATUS) {
            if (!DEALING_WITH_DAEMON(&conf)) {
                fprintf(stderr, "\"status\" command support only for daemon "
                                "with pidfile\n");
                goto status_stop_exiting;
            } else {
                pid_t __daemon_pid = readPidFile(conf.child.pidfile);
                if (0 > __daemon_pid) {
                     goto status_stop_exiting;
                }

                if (0 > kill(__daemon_pid, 0)) {
                    if (errno == ESRCH) {
                        fprintf(stdout, "\"%s\" process is not running\n",
                                conf.child.base_name);
                    } else {
                        perror("kill(respawner_pid, 0)");
                        goto status_stop_exiting;
                    }
                } else {
                    fprintf(stdout, "\"%s\" process is running\n",
                            conf.child.base_name);
                    self_exit_status = EXIT_SUCCESS;
                }
            }
        }

    status_stop_exiting:
        // freeConf(&conf);
        exit(self_exit_status);
    }

    // отрабатываем команду "start"
    if (conf.daemonize) {
        if (daemon(0, 0)) {
            perror("daemon()");
            goto laFinite;
        }
    }
    respawner_pid = getpid();

    if (conf.daemonize) {
        closelog();
        openlog(PROJECT_NAME, LOG_PID, LOG_DAEMON);
    }

    syslog(LOG_INFO, "started (pid %d), command: %s %s", respawner_pid,
           getCommandName(conf.comm), conf.child.base_name);

    if (!respawnerPidFileCreate(conf.child.base_name, respawner_pid)) {
        goto laFinite;
    }

    epoll_fdes = epoll_create1(0);
    if (0 > epoll_fdes) {
        perror("epoll_create1()");
        goto laFinite;
    }

    signal_fdes = initSignal(epoll_fdes);
    if (0 > signal_fdes) {
        goto laFinite;
    }

    timer_fdes = initTimer();
    if (0 > timer_fdes) {
        goto laFinite;
    }

    // Main loop

    // (firstly) start child process
    child_pid = execProc(conf.child.path_name, conf.child.argv);
    if (child_pid < 0) {
        // unrecoverable fork() of execve() error
        syslog(LOG_ERR, "%s start failed", conf.child.base_name);
        goto laFinite;
    }

    syslog(LOG_NOTICE, "%s started, pid %d", conf.child.base_name, child_pid);

    // main respawner loop
    do {
        if (0 > child_pid && 0 > daemon_pid) {
            // have not child process
            if (0 > timerArm(timer_fdes, epoll_fdes, conf.respawn.timeout)) {
                goto laFinite;
            }
        } else if (respawnNbr > 0) {
            if (0 > timerDisarm(timer_fdes, epoll_fdes)) {
                goto laFinite;
            }
        }

        nfds = epoll_wait(epoll_fdes, events, MAX_EVENTS, -1);
        if (0 > nfds) {
            perror("epoll_wait()");
            goto laFinite;
        }

        for (n = 0; IS_CONTINUE_RESPAWN && n < nfds; n += 1) {
            if (events[n].data.fd == signal_fdes) {
                ret = handleSignalEvents(signal_fdes, events[n].events,
                                         signal_callback);
                if (0 > ret) {
                    goto laFinite;
                }
            } else if (events[n].data.fd == timer_fdes) {
                if (0 > handleTimerEvents(timer_fdes, events[n].events)) {
                    goto laFinite;
                }

                if (conf.respawn.max > 0 && respawnNbr > conf.respawn.max) {
                    respawnerLoop = true; // EXIT NORMAL
                    break;
                }

                if (0 > child_pid) {
                    child_pid = execProc(conf.child.path_name, conf.child.argv);
                    if (child_pid < 0) {
                        // unrecoverable fork() of execve() error
                        goto laFinite;
                    }
                    respawnNbr += 1;
                    syslog(LOG_NOTICE, "%s restarted(%d/%d), pid %d",
                           conf.child.base_name, respawnNbr, conf.respawn.max,
                           child_pid);
                }
            }
        }
    } while (IS_CONTINUE_RESPAWN);

// завершение работы

    self_exit_status =
        respawnerLoop == DO_EXIT_FAILURE ? EXIT_FAILURE : EXIT_SUCCESS;

laFinite:
    CLOSE_FDESCRIPTOR(timer_fdes);
    CLOSE_FDESCRIPTOR(signal_fdes);
    CLOSE_FDESCRIPTOR(epoll_fdes);

    // freeConf(&conf);

    respawnerPidFileDelete();

    syslog(LOG_INFO, "stopped");
    closelog();

    exit(self_exit_status);
}
