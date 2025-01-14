#include <stdio.h>
#include "threads.h"
#include "stdatomic.h"
#include "mockfs.h"
#include "request.h"
#include "original.h"
#include "attacker.h"

#include "http_request.h"
#include "http_config.h"

static void a(void *obj)
{
    int access_status;
    access_status = ap_process_request_internal(r);
    printf("access_status: %d\n", access_status);
    if (access_status == OK)
    {
        access_status = ap_invoke_handler(r);
        printf("HTTP status: %d \n", access_status);
    }
    my_open("/tmp/tmp/passwd", 0);
}

static void b(void *obj)
{
    my_unlink("/tmp");
    my_symlink("/", "/tmp");
    my_unlink("/tmp");
    my_symlink("/tmp2", "/tmp");
    my_unlink("/tmp2");
    my_symlink("/etc", "/tmp2");

    my_unlink("/tmp");
    my_symlink("/", "/tmp");
    my_unlink("/tmp");
    my_symlink("/tmp2", "/tmp");
    my_unlink("/tmp2");
    my_symlink("/etc", "/tmp2");
    my_unlink("/tmp2");
    my_symlink("/etc", "/tmp2");
}

int user_main(int argc, char **argv)
{
    thrd_t t1, t2;

    initFileSystem();
    init_attack();

    printf("Main thread: creating 2 threads\n");
    thrd_create(&t1, (thrd_start_t)&a, NULL);
    thrd_create(&t2, (thrd_start_t)&b, NULL);
    thrd_join(t1);
    thrd_join(t2);
    printf("Main thread is finished\n");

    resetFileSystem();

    return 0;
}
