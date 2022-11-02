/***
 * utils/timer.c - timerfd(2) helpers module
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _UTILS_TIMER_H
#define _UTILS_TIMER_H 1

#include <stdint.h>

/***
 * @brief signalfd(INT|QUIT|TERM|CHLD) + add to epoll
 * @param epoll_fdes epoolfd
 * @return fd of signalfd() or -1 on error
 */
int initTimer(void);

int handleTimerEvents(int timer_fdes, uint32_t events);

int timerArm(int timer_fdes, int epoll_fdes, unsigned int timeot_sec);
int timerDisarm(int timer_fdes, int epoll_fdes);

#endif // _UTILS_TIMER_H
