#/bin/sh

set -x
set -e

RESPAWNER_BIN="../../src/respawner"
SINGLE_PROC="@PROJECT_SOURCE_DIR@/test/mock/forever.sh"
cat $SINGLE_PROC

# запуск обычного зацикленного процесса - не демона
$RESPAWNER_BIN start $SINGLE_PROC
sleep 3
pkill -9 forever.sh
sleep 5
$RESPAWNER_BIN stop $SINGLE_PROC
