/***
 * onsig-behaviour.h - module for signal handlers, TERM and CHLD
 *                     implement most of the respawn behavior logic
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _ONSIG_BEHAVE_H
#define _ONSIG_BEHAVE_H 1

#include <stdbool.h>
#include <sys/signalfd.h>

// Обработчики сигналов, по каждому процессу-источнику:
//   respawner(self), daemon started child proc,
//   daemon child proc, single (no daemon) proc
// Все ф-ции возвращают false если _дальнейшая_ обработка
// других полученных сигналов уже бессмысленна
bool onSelfTERM(void);
bool onDaemonStarterCHLD(const conf_t *conf, const struct signalfd_siginfo *siginfo);
bool onDaemonCHLD(const conf_t *conf, const struct signalfd_siginfo *siginfo);
bool onChildCHLD(const conf_t *conf, const struct signalfd_siginfo *siginfo);

#endif // _ONSIG_BEHAVE_H
