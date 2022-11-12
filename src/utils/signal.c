/***
 * utils/signal.c - signalfd(2) helpers module
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
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>

#include "signal.h"

#define MAX_PENDING_SIGNALS 100

static int getPendingSignals(int signal_fdes,
                             struct signalfd_siginfo pendingSignals[],
                             int pendingSignalsLength);

int initSignal(int epoll_fdes)
{
    int fd;
    sigset_t mask;
    struct epoll_event ev = {0};

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    fd = signalfd(-1, &mask, SFD_NONBLOCK);

    if (0 > fd) {
        perror("signalfd()");
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (0 > epoll_ctl(epoll_fdes, EPOLL_CTL_ADD, fd, &ev)) {
        perror("epoll_ctl(signalfd)");
        close(fd);
        return -1;
    }

    return fd;
} // initSignal()

int handleSignalEvents(int signal_fdes, uint32_t events,
                       signal_callback_t signal_callback)
{
    int pendingSignalsNbrs = 0;
    // We don't use MT
    struct signalfd_siginfo pendingSignals[100];
    const struct signalfd_siginfo *pendingSignal;
    int i;

    if (events & (EPOLLERR | EPOLLHUP)) {
        syslog(LOG_ERR, "signalfd return err|hup event\n");
        return -1;
    }
    if (events & EPOLLIN) {
        pendingSignalsNbrs =
            getPendingSignals(signal_fdes, pendingSignals, MAX_PENDING_SIGNALS);
        if (0 > pendingSignalsNbrs) {
            return -1;
        }
        for (i = 0; i < pendingSignalsNbrs; i += 1) {
            pendingSignal = &pendingSignals[i];
            if (!signal_callback(pendingSignal)) {
                break;
            }
        }
    }

    return pendingSignalsNbrs;
} // handleSignalEvents()

static int getPendingSignals(int signal_fdes,
                             struct signalfd_siginfo pendingSignals[],
                             int pendingSignalsLength)
{
    static struct signalfd_siginfo tmp[MAX_PENDING_SIGNALS];
    ssize_t rred;
    unsigned int u, signaledNbr;
    int pendingSignalsCount;

    for (pendingSignalsCount = 0; pendingSignalsCount < pendingSignalsLength;
         pendingSignalsCount += 1) {
        rred = read(signal_fdes, tmp, sizeof(tmp));
        if (0 > rred) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("read(signalfd)");
                return -1;
            }
        }
        signaledNbr = rred / sizeof(struct signalfd_siginfo);
        for (u = 0; u < signaledNbr; u += 1) {
            pendingSignals[pendingSignalsCount] = tmp[u];
        }
    }

    return pendingSignalsCount;
} // getPendingSignals()
