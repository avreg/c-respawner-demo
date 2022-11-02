/***
 * utils/timer.h - timerfd(2) helpers module
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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "timer.h"

static bool timerIsArm = false;

static int timer_set(int timer_fdes, int epoll_fdes, unsigned int timeot_sec);

int initTimer(void)
{
    int fd;

    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (0 > fd) {
        perror("timerfd_create(CLOCK_MONOTONIC)");
        return -1;
    }

    return fd;
} // initTimer()

int timerArm(int timer_fdes, int epoll_fdes, unsigned int timeot_sec)
{
    if (timerIsArm) {
        return 0; // success
    }
    return timer_set(timer_fdes, epoll_fdes, timeot_sec);
} // timerArm()

int timerDisarm(int timer_fdes, int epoll_fdes)
{
    if (!timerIsArm) {
        return 0; // success
    }
    return timer_set(timer_fdes, epoll_fdes, 0);
} // timerDisarm()

int handleTimerEvents(int timer_fdes, uint32_t events)
{
    uint64_t exp;
    ssize_t s;

    if (events & (EPOLLERR | EPOLLHUP)) {
        syslog(LOG_ERR, "timerfd return err|hup event");
        return -1;
    }
    if (events & EPOLLIN) {
        s = read(timer_fdes, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            perror("read(timerfd)");
            return -1;
        }
    }

    return 0;
} // handleTimerFdEvents()

static int timer_set(int timer_fdes, int epoll_fdes, unsigned int timeot_sec)
{
    struct epoll_event ev;
    struct itimerspec new_value;

    memset(&new_value, 0, sizeof(new_value));
    new_value.it_value.tv_nsec = 0;
    new_value.it_value.tv_sec = timeot_sec;

    if (0 > timerfd_settime(timer_fdes, 0, &new_value, NULL)) {
        perror("timerfd_settime()");
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = timer_fdes;
    if (0 > epoll_ctl(epoll_fdes,
                      timeot_sec > 0 ? EPOLL_CTL_ADD : EPOLL_CTL_DEL,
                      timer_fdes, &ev)) {
        perror(timeot_sec > 0 ? "epoll_ctl(timerfd, ADD)"
                              : "epoll_ctl(timerfd, DEL)");
        return -1;
    }

    timerIsArm = timeot_sec > 0;

    return 0;
} // timer_set()
