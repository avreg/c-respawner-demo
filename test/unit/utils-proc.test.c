#include <limits.h>
#include <stdio.h>
#include <assert.h>

#include "unity.h"
#include "utils/proc.h"

static void readPidFile_success(void)
{
    // TODO: use mktemp(3)
    #define PIDPATH "/tmp/pidfile.test.pid"
    char buf[20];
    int buf_len, wrote;

    FILE *pidfile = fopen(PIDPATH, "w");
    assert(pidfile);

    buf_len = snprintf(buf, sizeof(buf), "%d\n", INT_MAX);
    assert(buf_len > 1);

    wrote = fwrite((const void *)buf, buf_len, 1, pidfile);
    assert(1 == wrote);
    fclose(pidfile);

    pid_t p = readPidFile(PIDPATH);
    TEST_ASSERT_EQUAL_INT(INT_MAX, p);
} // readPidFile_success()

static void readPidFile_failure(void)
{
    pid_t p = readPidFile("/NON_EXISTENT_PID_FILE");
    TEST_ASSERT_EQUAL_INT(-1, p);
} // readPidFile_failure()

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(readPidFile_success);
    RUN_TEST(readPidFile_failure);
    return UNITY_END();
}

void setUp(void) {}

void tearDown(void) {}
