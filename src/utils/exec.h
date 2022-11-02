/***
 * utils/exec.h - exec utility helpers
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _UTILS_EXEC_H_
#define _UTILS_EXEC_H_ 1

#include <unistd.h>

/**
 * @brief Запускает execve(2)/execvp(3) исполняемый файл
 * - потомок, за которым нужно следить.
 *
 * @param pathname путь исполняемого файла - потомка
 * @param argv     argv для исполняемого файла - потомка
 * @return pid_t   возвращается pid запущенного потомка или -1 при ошибке
 */
pid_t execProc(const char *pathname, char *const argv[]);

#endif // _UTILS_EXEC_H_
