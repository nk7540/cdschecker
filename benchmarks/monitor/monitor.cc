#include <stdio.h>
#include "threads.h"
#include "stdatomic.h"
#include "mockfs.h"
#include "request.h"

#include "http_request.h"
#include "http_config.h"

static void a(void *obj)
{
    int access_status;
    // r->unparsed_uri = "/etc/passwd";
    // r->uri = "/etc/passwd";
    access_status = ap_process_request_internal(r);
    printf("access_status: %d\n", access_status);
    // if (access_status == OK)
    // {
    //     access_status = ap_invoke_handler(r);
    //     printf("HTTP status: %d \n", access_status);
    // }
}

static void b(void *obj)
{
    my_open("file1.html", 0);
    // my_link("file1.html", "passwd");
}

int user_main(int argc, char **argv)
{
    thrd_t t1, t2;

    initFileSystem();

    printf("Main thread: creating 2 threads\n");
    thrd_create(&t1, (thrd_start_t)&a, NULL);
    thrd_create(&t2, (thrd_start_t)&b, NULL);
    thrd_join(t1);
    thrd_join(t2);
    printf("Main thread is finished\n");

    resetFileSystem();

    return 0;
}
