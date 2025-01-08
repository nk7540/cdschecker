#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "original.h"

#define BASE_DIR "/tmp/test"

void create_directories()
{
    char path[1024] = BASE_DIR;
    for (int i = 1; i <= 10; i++)
    {
        snprintf(path + strlen(BASE_DIR), sizeof(path) - strlen(BASE_DIR), "/dir%d", i);
        if (mkdir(path, 0755) == -1 && errno != EEXIST)
        {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

clock_t cpu_time_start;
clock_t cpu_time_end;
double sec_start;
double sec_end;
double result_time;

// int measure_duration(void **func)
// {
//     func();
// }

double res_safe[10][50];
double res_unsafe[10][50];
double ave_safe[10];
double ave_unsafe[10];

int main(int argc, char **argv)
{
    char path[1024];

    // Ensure the base directory exists
    create_directories();

    for (int k = 1; k <= 50; k++)
    {
        for (int i = 1; i <= 10; i++)
        {
            snprintf(path, sizeof(path), BASE_DIR);

            // Construct the path with `i` components
            for (int j = 1; j <= i; j++)
            {
                snprintf(path + strlen(path), sizeof(path) - strlen(path), "/dir%d", j);
            }

            printf("Trying to open: %s\n", path);
            cpu_time_start = clock();
            sec_start = (double)cpu_time_start * 1000000 / CLOCKS_PER_SEC;

            // Attempt to open the constructed path
            int fd = safe_open(path, O_RDONLY, NULL);

            cpu_time_end = clock();
            sec_end = (double)cpu_time_end * 1000000 / CLOCKS_PER_SEC;
            result_time = sec_end - sec_start;
            res_safe[i][k] = result_time;
            printf("Duration: %f\n", result_time);

            if (fd == -1)
            {
                perror("open");
            }
            else
            {
                printf("Successfully opened: %s\n", path);
                close(fd);
            }

            // unsafe

            printf("Trying to unsafe_open: %s\n", path);
            cpu_time_start = clock();
            sec_start = (double)cpu_time_start * 1000000 / CLOCKS_PER_SEC;

            // Attempt to open the constructed path
            fd = unsafe_open(path, O_RDONLY);

            cpu_time_end = clock();
            sec_end = (double)cpu_time_end * 1000000 / CLOCKS_PER_SEC;
            result_time = sec_end - sec_start;
            res_unsafe[i][k] = result_time;
            printf("Duration: %f\n", result_time);

            if (fd == -1)
            {
                perror("open");
            }
            else
            {
                printf("Successfully opened: %s\n", path);
                close(fd);
            }
        }
    }

    for (int i = 1; i <= 10; i++)
    {
        double sum_safe = 0;
        double sum_unsafe = 0;
        for (int k = 1; k <= 50; k++)
        {
            sum_safe += res_safe[i][k];
            sum_unsafe += res_unsafe[i][k];
        }
        ave_safe[i] = sum_safe / 50;
        ave_unsafe[i] = sum_unsafe / 50;
        printf("ave_safe[%d] = %f\n", i, ave_safe[i]);
        printf("ave_unsafe[%d] = %f\n", i, ave_unsafe[i]);
    }

    return 0;
}
