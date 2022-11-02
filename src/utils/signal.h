/***
 * utils/signal.h - signalfd(2) helpers module
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _UTILS_SIGNAL_H_
#define _UTILS_SIGNAL_H_ 1

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/signalfd.h>
#include <sys/types.h>

/***
 * @brief signalfd(INT|QUIT|TERM|CHLD) + add to epoll
 * @param epoll_fdes epoolfd
 * @return fd of signalfd() or -1 on error
 */
int initSignal(int epoll_fdes);

typedef bool (*signal_callback_t)(const struct signalfd_siginfo *siginfo);

/**
 * @brief Обработчик событий signalfd(2)
 *
 * @param signal_fdes      дескриптор signalfd(2)
 * @param events           события от epoll(2)
 * @param signal_callback  каллбэк обработчик единичного события
 * @return int             -1 при ошибке или кол-во обработанных событий
 */
int handleSignalEvents(int signal_fdes, uint32_t events,
                       signal_callback_t signal_callback);

#endif // _UTILS_SIGNAL_H_
