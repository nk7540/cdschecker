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
#define MAX_PATH_LENGTH 10
#define ITERATION 500

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

double res[4][MAX_PATH_LENGTH][ITERATION];
double ave[4][MAX_PATH_LENGTH];

int main(int argc, char **argv)
{
    char path[MAX_PATH_LENGTH];

    // Ensure the base directory exists
    create_directories();

    for (int p = 1; p <= 4; p++)
    {
        for (int k = 1; k <= ITERATION; k++)
        {
            for (int i = 1; i <= MAX_PATH_LENGTH; i++)
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

                int fd;
                switch (p)
                {
                case 1:
                    fd = unsafe_open(path, O_RDONLY);
                    break;
                case 2:
                    fd = krace(path, NULL);
                    break;
                case 3:
                    fd = atomic_krace(path);
                    break;
                case 4:
                    fd = safe_open(path, O_RDONLY, NULL);
                    break;
                default:
                    break;
                }

                cpu_time_end = clock();
                sec_end = (double)cpu_time_end * 1000000 / CLOCKS_PER_SEC;
                result_time = sec_end - sec_start;
                res[p][i][k] = result_time;
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
    }

    for (int p = 1; p <= 4; p++)
    {
        switch (p)
        {
        case 1:
            printf("y1 = [");
            break;
        case 2:
            printf("y2 = [");
            break;
        case 3:
            printf("y3 = [");
            break;
        case 4:
            printf("y4 = [");
            break;
        default:
            break;
        }
        for (int i = 1; i <= MAX_PATH_LENGTH; i++)
        {
            double sum = 0;
            for (int k = 1; k <= ITERATION; k++)
            {
                sum += res[p][i][k];
            }
            ave[p][i] = sum / ITERATION;
            printf("%f, ", ave[p][i]);
        }
        printf("]\n");
    }

    return 0;
}
