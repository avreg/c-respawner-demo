/***
 * conf.c - module to parsing command line
 *
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: Copyright 2022 Andrey Nikitin <nik-a@mail.ru>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include <respawner-config.h>
#endif

#include <assert.h>
#include <libgen.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include "conf.h"

#define PID_PATH_MAX 255

// clang-format off
#define FREEPCHAR(s) do { \
    if ((s) != NULL) {    \
        free(s);          \
        s = NULL;         \
    }                     \
} while (0);
// clang-format on

static enum APP_COMMAND parseCommand(const char *arg);
static void printUsage(const char *argv0);
static void parseSuccessExitStatus(const char *optarg, conf_t *conf);

#if 0
void freeConf(conf_t *conf)
{
    assert(conf);
    if (conf->child.base_name) {
        free(conf->child.base_name);
    }
} // freeConf()
#endif

bool getConf(int argc, char *argv[], conf_t *conf)
{
    int opt;

    int genPidFile = 0;
    static char pidFileStatic[PID_PATH_MAX];
    static char rpath[PATH_MAX];
    char *optionalArg;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"respawn-limit", required_argument, 0, 'n'},
            {"respawn-timeout", required_argument, 0, 't'},
            {"pidfile", optional_argument, 0, 'p'},
            {"success-exit-status", required_argument, 0, 'e'},
            {"help", 0, 0, 'h'},
            {0}};

        opt =
            getopt_long(argc, argv, "Dhn:t:e:p::", long_options, &option_index);
        if (0 > opt) {
            break;
        }

        switch (opt) {
        case 'D':
            conf->daemonize = false;
            break;
        case 'n':
            conf->respawn.max = atoi(optarg);
            break;
        case 't':
            conf->respawn.timeout = atoi(optarg);
            break;
        case 'p':
            optionalArg = argv[optind];
            if (optionalArg && optionalArg[0] == '/') {
                conf->child.pidfile = optionalArg;
                optind += 1;
            } else {
                genPidFile = 1;
            }
            break;
        case 'e':
            parseSuccessExitStatus(optarg, conf);
            break;
        default: /* '?' */
            printUsage(argv[0]);
            return false;
        }
    }

    int i;
    for (i = optind; i < argc; i++) {
        int non_opt_arg_pos = i - optind;
        char *non_opt_arg = argv[i];

        if (non_opt_arg_pos == 0) {
            // command: start|stop|status
            conf->comm = parseCommand(non_opt_arg);
        } else if (non_opt_arg_pos == 1) {
            // child exec pathname
            if (non_opt_arg[0] == '.') {
                conf->child.path_name = realpath(non_opt_arg, rpath);
            } else {
                conf->child.path_name = non_opt_arg;
            }
            char *buf = strdup(conf->child.path_name);
            assert(buf);
            conf->child.base_name = basename(buf); // FIXME: valgring possibly lost warning
            conf->child.argv[0] = non_opt_arg;
            conf->child.argc = 1;
        } else {
            // child exec args
            if (conf->child.argc < MAX_ARGS) {
                conf->child.argv[conf->child.argc++] = non_opt_arg;
            }
        }
    }

    if (conf->comm == COMM_INVAL || !conf->child.path_name) {
        printUsage(argv[0]);
        return false;
    }

    if (genPidFile) {
        snprintf(pidFileStatic, sizeof(pidFileStatic), "/run/%s.pid",
                 conf->child.base_name);
        conf->child.pidfile = pidFileStatic;
    }

    return true;
} // getConf()

static enum APP_COMMAND parseCommand(const char *arg)
{
    if (!arg || arg[0] == '\0') {
        return COMM_INVAL;
    }

    if (!strcmp(arg, "start")) {
        return COMM_START;
    } else if (!strcmp(arg, "status")) {
        return COMM_STATUS;
    } else if (!strcmp(arg, "stop")) {
        return COMM_STOP;
    }

    return COMM_START;
} // parseCommand()

static void printUsage(const char *argv0)
{
    assert(argv0);
    char *argv0_copy = strdup(argv0);
    assert(argv0_copy);

    // clang-format off
    fprintf(stderr,
            "Usage: %s [options] start|stop|status child-pathname "
            "[-- child-args ...]\n"
            "where options:\n"
            "  -D                             do not daemonize(default) respawner\n"
            "  -n|--respawn-limit NUM         respawn max\n"
            "  -t|--respawn-timeout SEC       respawn timeout\n"
            "  -p|--pidfile [PIDFILE]         pidfile or, if empty,\n"
            "                                 /{run,tmp}/child-pathname.pid\n"
            "  -e|--success-exit-status LIST  CSV list of success\n"
            "                                 exit status codes\n"
            "\n",
            basename(argv0_copy));
    // clang-format on
    free(argv0_copy);
} // printUsage()

static void parseSuccessExitStatus(const char *optarg, conf_t *conf)
{
    char *src;
    const char *tok;

    if (!optarg || optarg[0] == '\0') {
        return;
    }

    src = strdup(optarg);
    if (!src) {
        perror("strdup(optarg)");
        return;
    }

    for (tok = strtok(src, ",;"); tok && *tok; tok = strtok(NULL, ",;\n")) {
        conf->child.successExitStatuses[conf->child.successExitStatusesNbr++] =
            atoi(tok);
    }

    free(src);

    return;
} // parseSuccessExitStatus()

bool isProcSuccessExitStatus(const conf_t *conf, int status)
{
    if (conf->child.successExitStatusesNbr <= 0) {
        return (status == 0);
    }

    for (int i = 0; i < conf->child.successExitStatusesNbr; i += 1) {
        if (status == conf->child.successExitStatuses[i]) {
            return true;
        }
    }

    return false;
} // isProcSuccessExitStatus()
