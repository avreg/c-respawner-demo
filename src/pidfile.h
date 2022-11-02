/***
 * pidfile.h - module with helpers to create, read and remove
 *             self-app pid file:
 *               /run/respawner-{CHILD}.pid - if run as root
 *             or
 *               /tmp/respawner-{CHILD}.pid - if run as non root
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _PIDFILE_H
#define _PIDFILE_H 1

#include <stdbool.h>
#include <unistd.h>

// хелперы своего (respwanwer) pidfile
bool respawnerPidFileCreate(const char *childBaseName, pid_t pid);
bool respawnerPidFileGet(const char *childBaseName, pid_t *pid);
bool respawnerPidFileDelete(void);

#endif // _PIDFILE_H
