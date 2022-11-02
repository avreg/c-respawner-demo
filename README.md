# Тестовое задание Linux respawner (C)

## ТЗ

```
> xxxxx, в продолжение разговора с коллегами, направляю вам тестовое задание.
>
> Написать приложение – watchdog для других приложений:
> Запуск произвольного ПО через watchdog
> Контроль состояния процесса, при необходимости – перезапуск с таймаутом 5 сек
> Предпочтительно использовать timerfd и signalfd
> Язык – С, использовать Linux api
> Предусмотреть возможность остановки процесса
> Приложение должно ставильно работать с lighttpd
> Пример запуска watchdog start lighttpd
```

## Решение

Прим.: Мне показалось ближе название respawner, т.к. watchdog для меня это когда что-то аппаратное используется.

Итак, реализуем программу respawer с частичным функционалом systemd.

### Окружение разработки

```
Язык: C(std11).
Система сборки: cmake/make (Makefile - враппер для запуска cmake с параметрами).
Компилятор: gdb.
ОС: Debian GNU/Linux 11 (bullseye).
```

### Сборка

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake

git clone https://github.com/avreg/c-respawner-demo.git
cd c-respawner-demo

make
```

Очистка:

```bash
make clean
```

### Описание

```
./build/src/respawner --help
Usage: respawner [options] start|stop|status child-pathname [-- child-args ...]
where options:
  -D                             do not daemonize(default) respawner
  -n|--respawn-limit NUM         respawn max
  -t|--respawn-timeout SEC       respawn timeout
  -p|--pidfile [PIDFILE]         pidfile or, if empty,
                                 /{run,tmp}/child-pathname.pid
  -e|--success-exit-status LIST  CSV list of success
                                 exit status codes
```

Программа имеет 3 командв/режима работы:

* start - запускает respawner loop для запуска и контроля целевого приложения.
* stop - останавливает respawner loop.
* status - возвращает в stdout и соотв. exit status состояние целевого приложения (работает или нет).

Все три режима начинаются с разбора аргументов командой строки и соотв. заполнения конфигурационной структуры.

#### start

Программа (по умолчанию) уходит в фоновое выполнение (демонизируется), запускает заданный исполняемый файл и входит в рабочий цикл алгоритма respawn.

Если исполняемый файл - простая программа (не демон), то котроль целевой программы происходит посредством ожидания обработки сигнала SIGCHLD, по pid-у потомка (после fork(2)).

Если исполняемый файл - демон и полностью отрывается от породивщего его respawner-процесса, тогда используется лёгкая (слушающая) трассировка процесса (PTRACE_SEIZE, по его pid-у, прочитанному с pidfile). Далее - так же как и простой программой, по SIGCHLD.

Если целевая программа (простая или демон) завершаются штатно (номрально, не аварийно), тогда завершается и сам процесс respawner.

Опциями командной строки можно задать лимиты перезапуска (таймаут и кол-во попыток), список нормальных кодов возврата (которые считать нормальными) и pidfile процесса демона (обязательно для демонов).

Можно было бы обойтись без pidfile для демонов, но пришлось бы делать обезьяний перебор procfs, который не "закрывал" бы такой сценарий - когда сразу запускается несколько инстансов демона и каждый отличается конфигами или опциями ком.строки.

#### stop

Все просто, посылаем TERM respawner-у по содержимому его pidfile, который `/run/respawner-{child-app}.pid` или `/tmp/respawner-{child-app}.pid`

#### status

Текущая версия работает только при использовании `--pidfile` (т.е. для демонов).
Проверка процесса производится `kill(pid, 0)` и проверкой возрата с учётом `errno == ESRCH`.

### Проверка

#### Простое приложение

```
cat ./test/mock/forever.sh

#!/bin/sh

success_exiting() {
    echo "$$ Got TERM sig, success exiting with zero status"
    exit 0
}

trap success_exiting TERM

echo "$$ Forever loop"

while true; do
    sleep 1
done

```

```bash
# запуск обычного зацикленного процесса - не демона
./build/src/respawner start ./test/mock/forever.sh
sleep 3
pkill -9 forever.sh
sleep 5
./build/src/respawner stop ./test/mock/forever.sh
```

Отражение этого сценария в логах (daemon.log на Debian или syslog на Ubuntu)

```
Nov  2 12:50:15 office respawner[1432138]: started (pid 1432138), command: start forever.sh
Nov  2 12:50:15 office respawner[1432138]: forever.sh started, pid 1432139
Nov  2 12:50:20 office respawner[1432138]: proc 1432139 killed by signo 9
Nov  2 12:50:25 office respawner[1432138]: forever.sh restarted(1/0), pid 1432312
Nov  2 12:50:38 office respawner[1432138]: kill(child pid 1432312)
Nov  2 12:50:39 office respawner[1432138]: proc 1432312 exited, status 0
Nov  2 12:50:39 office respawner[1432138]: stopped
```

#### Завершающее с ошибкой приложение

```
cat ./test/mock/single.sh

#!/bin/sh

EXIT_CODE=${1:-0}
DELAY=${2:-2}

success_exiting() {
    echo "$$ Got TERM sig, success exiting with zero status"
    exit 0
}

trap success_exiting TERM

echo "$$ Hello, wait ${DELAY} sec"

sleep ${DELAY}

echo "$$ Bye, exit code ${EXIT_CODE}"

exit ${EXIT_CODE}
```

```bash
# запуск обычного зацикленного процесса - не демона
./build/src/respawner start ./test/mock/single.sh -- 1 3
sleep 15
./build/src/respawner stop single.sh
```

В логах:
```
Nov  2 13:00:15 office respawner[1442325]: started (pid 1442325), command: start single.sh
Nov  2 13:00:15 office respawner[1442325]: single.sh started, pid 1442326
Nov  2 13:00:18 office respawner[1442325]: proc 1442326 exited, status 1
Nov  2 13:00:23 office respawner[1442325]: single.sh restarted(1/0), pid 1442479
Nov  2 13:00:26 office respawner[1442325]: proc 1442479 exited, status 1
Nov  2 13:00:31 office respawner[1442325]: single.sh restarted(2/0), pid 1442546
Nov  2 13:00:34 office respawner[1442325]: proc 1442546 exited, status 1
Nov  2 13:00:39 office respawner[1442325]: single.sh restarted(3/0), pid 1442729
Nov  2 13:00:42 office respawner[1442325]: proc 1442729 exited, status 1
Nov  2 13:00:44 office respawner[1442325]: stopped
```

#### Демон lighttpd

Прим.:

* запускать нужно от рута или с пом. sudo.
* демон делает несколько fork(2) (clone) и полностью отрывается от respawner-а, мониторим по его pidfile-у.
* нормальное завершение по TERM, как не странно, у него 1 (exit status), это тоже приходится учитывать.

```bash
#!/bin/sh

sudo -E ./build/src/respawner \
    start -e 0,1 -p \
    lighttpd -- -f /etc/lighttpd/lighttpd.conf

# ждём запуска и проверяем статус - работает
sleep 3
sudo ./build/src/respawner -p status lighttpd
# stdout: "lighttpd" process is running

# аварийно завершаем lighttpd и проверяем статус - не работает
sudo pkill -9 lighttpd && \
    sleep 1 && \
    sudo ./build/src/respawner status -p lighttpd      
# stdout: "lighttpd" process is not running

# ждём перезапуска и проверяем статус - работает
sleep 7
sudo ./build/src/respawner status -p lighttpd
# stdout: "lighttpd" process is running

# нормально оставливаем lighttpd и соотв. остановится и respawner
sudo pkill -TERM lighttpd
```

В логах:

```
Nov  2 13:49:15 office respawner[1501092]: started (pid 1501092), command: start lighttpd
Nov  2 13:49:15 office respawner[1501092]: lighttpd started, pid 1501093
Nov  2 13:49:15 office respawner[1501092]: proc 1501093 exited, status 0
Nov  2 13:49:15 office respawner[1501092]: start trace lighttpd daemon pid 1501102
Nov  2 13:49:18 office respawner[1501092]: proc 1501102 killed by signo 9
Nov  2 13:49:23 office respawner[1501092]: lighttpd restarted(1/0), pid 1501206
Nov  2 13:49:23 office respawner[1501092]: proc 1501206 exited, status 0
Nov  2 13:49:23 office respawner[1501092]: start trace lighttpd daemon pid 1501214
Nov  2 13:49:26 office respawner[1501092]: proc 1501214 exited, status 1
Nov  2 13:49:26 office respawner[1501092]: stopped
```

### Ограничения/недоработки

* Возможно это и имелось ввиду ("linux api"), но если речь про исключительное использование системных вызовов (2) вместо библиотечных (3), то в целом придерживался (старался), но не прям чтобы 100% (с учётом достаточно большого затраченного времени). Например, демонизация сделана через libc-шный daemon(3), ибо расписывать долго, а кроме fork(2) и setsid(2) там ничего интересного, тем более fork(2) все равно есть в коде на запуске потомка. Где-то есть fopen(3) вместо open(2), который также есть в др. местах.
* Команда "status" поддерживается только при использовании pidfile (-p), иначе пришлось бы использовать клиент-сервеную архитектуру приложения (respawnerctl и respawnersrv) с взаимодействием через unix socket, например. Ну, или глупый и неточный (если имя одинаковое или несколько инстансов) перебор procfs proc(5). Можно, кстати, немного доработать и сохранять pid простых процессов тоже, тогда status будет работать для всех вариантов целевых процессов.
* Комментирвание кода. Имена переменные и ф-ций стараюсь делать "говорящими". Комментировал только условно сложные для понимания алгоритмы и куски кода. Не пинайте сильно.

ptrace() без pidfile потомка использовать не получится (или будет оч. сложно),
например, потому что lighttpd 3 (или 2) раза fork-ается (судя по strace)

## TODO

* прогнать valgrind-ом;
* добавить в сборку функциональные тесты;
* debian/rpm package.

Прим.: не могу и не хочу больше затягивать и тратить своё и ваше время, поэтому тесты и пакеты сделаю если только:

* решение задачи в целом устраивает и
* посчитаете важным.
