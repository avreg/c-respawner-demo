/***
 * utils/proc.h - process-level utility helpers
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _UTILS_PROC_H_
#define _UTIL_PROC_H_ 1

#include <unistd.h>

enum PROC_STATES {
    PROC_UNKNOWN = 0,
    PROC_STARTED,
    PROC_TERMINATED,
    PROC_SIGNALED,
    PROC_STOPPED,
    PROC_CONTINUED
};

enum PROC_STATES parse_wstatus(int wstatus, int *status, int *signno);
pid_t readPidFile(const char *pidfilepath);

void procParking(pid_t pid);

#endif // _UTILS_PROC_H_
