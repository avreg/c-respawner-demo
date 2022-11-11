
configure_file(forever.test.sh forever.test.sh @ONLY)

add_test(
    NAME respawnForeverTestFn
    COMMAND ./forever.test.sh
)
