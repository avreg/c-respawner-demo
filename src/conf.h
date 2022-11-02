/***
 * conf.h - module to parsing command line
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _CONF_H_
#define _CONF_H_ 1

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

enum PROGRAM_STATUS { INIT, WAIT_CHILD, WAIT_DAEMON };
enum RESPAWNER_LOOP { DO_EXIT_FAILURE, DO_EXIT_SUCCESS, CONTINUE_RESPAWN };

/// @brief respawn mode settings
typedef struct respawn_s {
    // TODO: use array of timeouts ex: [1,2,3,5,10,60]
    unsigned int timeout; /// respawn timeout in sec
    unsigned int max;     /// max number of unsuccessfully respawn
} respawn_t;

#define MAX_ARGS 100
#define MAX_SUCCESS_STATUS_CODES 3

typedef struct child_exec_s {
    int argc;
    char *argv[MAX_ARGS];

    char *path_name;
    char *base_name;

    char *pidfile; ///< pid-файл процесса потомка

    /**
     * Нормальные коды возврата, например lighttpd,
     * при вполне себе штатном завершении по TERM,
     * возвращает 1.
    */
    int successExitStatuses[MAX_SUCCESS_STATUS_CODES];
    int successExitStatusesNbr;
} child_exec_t;

enum APP_COMMAND { COMM_INVAL = 0, COMM_START, COMM_STATUS, COMM_STOP };

typedef struct conf_s {
    bool daemonize;
    child_exec_t child;
    respawn_t respawn;

    enum APP_COMMAND comm;

} conf_t;

static inline char *getCommandName(enum APP_COMMAND cmd)
{
    switch (cmd) {
    case COMM_START:
        return "start";
    case COMM_STATUS:
        return "status";
    case COMM_STOP:
        return "stop";
    default:
        return "unknown";
    }
}

/**
 * @brief Парсит командную строку и заполняет conf_t структуру.
 *
 * @param argc
 * @param argv
 * @param conf    conf_t структура, опр. дальнейшее поведение программы.
 * @return true   успех
 * @return false  ошибка
 */
bool getConf(int argc, char *argv[], conf_t *conf);
void freeConf(conf_t *conf);

bool isProcSuccessExitStatus(const conf_t *conf, int status);

#endif // _CONF_H_
