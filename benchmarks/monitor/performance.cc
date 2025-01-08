#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <vector>

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
int result_time;

int res[4][MAX_PATH_LENGTH][ITERATION];
int ave[4][MAX_PATH_LENGTH];
char path[256];

int main(int argc, char **argv)
{
    // Ensure the base directory exists
    create_directories();

    for (int p = 1; p <= 4; p++)
    {
        printf("open with p = %d\n", p);
        for (int i = 1; i <= MAX_PATH_LENGTH; i++)
        {
            for (int k = 1; k <= ITERATION; k++)
            {
                snprintf(path, sizeof(path), BASE_DIR);

                // Construct the path with `i` components
                for (int j = 1; j <= i; j++)
                {
                    snprintf(path + strlen(path), sizeof(path) - strlen(path), "/dir%d", j);
                }

                printf("Trying to open: %s\n", path);
                cpu_time_start = clock();

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
                result_time = (int)(cpu_time_end - cpu_time_start);
                printf("res\n");
                res[p - 1][i - 1][k - 1] = result_time;
                printf("Duration: %d\n", result_time);

                if (fd == -1)
                {
                    perror("open");
                }
                else
                {
                    printf("Successfully opened\n");
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
            int sum = 0;
            for (int k = 1; k <= ITERATION; k++)
            {
                sum += res[p - 1][i - 1][k - 1];
            }
            ave[p - 1][i - 1] = sum / ITERATION;
            printf("%d, ", ave[p - 1][i - 1]);
        }
        printf("]\n");
    }

    return 0;
}
