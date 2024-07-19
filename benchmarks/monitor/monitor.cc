#include "stdio.h"
#include "threads.h"
#include "stdatomic.h"
#include "mockfs.h"

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"

FileSystem fs;

static void a(void *obj)
{
    request_rec req = {
        .unparsed_uri = "/file1.html",
        .uri = "/file1.html",
    };
    ap_process_request_internal(&req);
}

static void b(void *obj)
{
}

int user_main(int argc, char **argv)
{
    thrd_t t1, t2;

    printf("Main thread: creating 2 threads\n");
    thrd_create(&t1, (thrd_start_t)&a, NULL);
    thrd_create(&t2, (thrd_start_t)&b, NULL);

    thrd_join(t1);
    thrd_join(t2);
    printf("Main thread is finished\n");

    return 0;
}
