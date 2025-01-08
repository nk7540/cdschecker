#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>

#include "original.h"

// Example access policy
const char *restricted_paths[] = {
    "/etc/passwd",
    "/etc/shadow",
    "/tmp/test/dir1",
    NULL // Sentinel to mark the end of the list
};

// Function to check if a path matches the restricted paths
int is_restricted_path(const char *path)
{
    for (const char **rp = restricted_paths; *rp != NULL; rp++)
    {
        if (strcmp(path, *rp) == 0)
        {
            return 1; // Match found
        }
    }
    return 0; // No match
}

int unsafe_open(const char *path, int flags, ...)
{
    if (!path || *path == '\0')
    {
        errno = ENOENT;
        return -1;
    }

    // Check if O_CREAT is specified and extract the mode if necessary
    mode_t mode = 0;
    if (flags & O_CREAT)
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    // Check against the access policy
    if (is_restricted_path(path))
    {
        errno = EACCES;
        return -1;
    }

    return open(path, flags, mode);
}

int safe_open(const char *path, int flags, char **final_resolved_path, ...)
{
    if (!path || *path == '\0')
    {
        errno = ENOENT;
        return -1;
    }

    // Check if O_CREAT is specified and extract the mode if necessary
    mode_t mode = 0;
    if (flags & O_CREAT)
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    // Duplicate the path to tokenize
    char *path_copy = strdup(path);
    if (!path_copy)
    {
        return -1;
    }

    // Start with the base directory
    int dir_fd = AT_FDCWD;
    if (path[0] == '/')
    { // Absolute path
        dir_fd = open("/", O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
        if (dir_fd < 0)
        {
            free(path_copy);
            return -1;
        }
    }

    char *token = strtok(path_copy, "/");
    char *next_token = NULL;
    char current_resolved_path[PATH_MAX] = {0};

    // Resolve the initial directory
    if (dir_fd != AT_FDCWD)
    {
        strcpy(current_resolved_path, "/");
    }
    else
    {
        if (!realpath(".", current_resolved_path))
        {
            free(path_copy);
            return -1;
        }
    }

    while (token)
    {
        next_token = strtok(NULL, "/");

        struct stat st;
        if (fstatat(dir_fd, token, &st, AT_SYMLINK_NOFOLLOW) < 0)
        {
            free(path_copy);
            if (dir_fd != AT_FDCWD)
                close(dir_fd);
            return -1;
        }

        int new_fd;

        if (S_ISLNK(st.st_mode))
        {
            // Component is a symlink; resolve it
            char link_target[PATH_MAX];
            ssize_t len = readlinkat(dir_fd, token, link_target, sizeof(link_target) - 1);
            if (len < 0)
            {
                free(path_copy);
                if (dir_fd != AT_FDCWD)
                    close(dir_fd);
                return -1;
            }
            link_target[len] = '\0';

            // Recursively resolve the symlink
            char *resolved_symlink_path = NULL;
            new_fd = safe_open(link_target, O_RDONLY | O_DIRECTORY | O_NOFOLLOW, &resolved_symlink_path);
            if (new_fd < 0)
            {
                free(path_copy);
                if (dir_fd != AT_FDCWD)
                    close(dir_fd);
                return -1;
            }

            // Combine resolved paths
            snprintf(current_resolved_path, sizeof(current_resolved_path), "%s/%s", resolved_symlink_path, next_token ? next_token : "");
            free(resolved_symlink_path);
        }
        else
        {
            // Update the resolved path
            snprintf(current_resolved_path, sizeof(current_resolved_path), "%s/%s", current_resolved_path, token);

            // Check against the access policy
            if (is_restricted_path(current_resolved_path))
            {
                free(path_copy);
                if (dir_fd != AT_FDCWD)
                    close(dir_fd);
                errno = EACCES;
                return -1;
            }

            int current_flags = O_RDONLY | O_NOFOLLOW | O_DIRECTORY;
            if (!next_token)
            {
                // Last component: apply user-specified flags
                current_flags = flags;
            }

            new_fd = openat(dir_fd, token, current_flags, mode);
            if (new_fd < 0)
            {
                free(path_copy);
                if (dir_fd != AT_FDCWD)
                    close(dir_fd);
                return -1;
            }
        }

        // Close the previous directory descriptor
        if (dir_fd != AT_FDCWD)
            close(dir_fd);
        dir_fd = new_fd;

        token = next_token;
    }

    // Copy the final resolved path
    if (final_resolved_path)
    {
        *final_resolved_path = strdup(current_resolved_path);
        if (!*final_resolved_path)
        {
            free(path_copy);
            if (dir_fd != AT_FDCWD)
                close(dir_fd);
            return -1;
        }
    }

    free(path_copy);
    return dir_fd;
}

int krace(const char *path, struct stat *s)
{
    int k = 5;
    struct stat buffer;
    if (access(path, R_OK) != 0)
    {
        errno = EACCES;
        return -1;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        errno = ENOENT;
        return -1;
    }
    /* This is the strengthening. */
    /*First, get the original inode. */
    if (fstat(fd, &buffer) != 0)
    {
        errno = ENOENT;
        return -1;
    }
    int orig_inode = buffer.st_ino;
    int orig_device = buffer.st_dev;
    /* Now, repeat the race. */
    /* File must be the same each time. */
    for (int i = 0; i < k; i++)
    {
        if (access(path, R_OK) != 0)
        {
            errno = EACCES;
            return -1;
        }
        int rept_fd = open(path, O_RDONLY);
        if (rept_fd < 0)
        {
            errno = ENOENT;
            return -1;
        }
        if (fstat(rept_fd, &buffer) != 0)
        {
            /* Return an error. */
            errno = ENOENT;
            return -1;
        }
        if (close(rept_fd) != 0)
        {
            errno = ENOENT;
            return -1;
        }
        if (orig_inode != buffer.st_ino)
        {

            errno = EACCES;
            return -1;
        }
        if (orig_device != buffer.st_dev)
        {
            errno = EACCES;
            return -1;
        }
    }
    return fd;
}

#define MAXPATHLEN 4096

char *chop_1st(char *path)
{
    char *slash = strchr(path, '/');
    if (!slash)
    {
        // No further segments; return NULL
        return NULL;
    }
    // Extract the first component (atom) and truncate path
    *slash = '\0';            // Terminate the first component
    char *suffix = slash + 1; // Remaining path
    return suffix;
}

int is_symlink(const char *path, char *target, struct stat *s, bool *is_sym)
{
    if (lstat(path, s) == -1)
    {
        return -1;
    }
    *is_sym = S_ISLNK(s->st_mode);
    if (*is_sym)
    {
        ssize_t len = readlink(path, target, MAXPATHLEN - 1);
        if (len == -1)
        {
            return -1;
        }
        target[len] = '\0';
    }
    return 0;
}

int atomic_krace(char *path)
{
    int fd;
    char *suffix, target[MAXPATHLEN];
    struct stat s;
    bool is_sym;

    // Handle absolute paths
    if (path[0] == '/')
    {
        if (chdir("/") == -1)
        {
            perror("chdir to root failed");
            return -1;
        }
        path++;
    }

    while (true)
    {
        suffix = chop_1st(path);
        if (is_symlink(path, target, &s, &is_sym) == -1)
        {
            perror("is_symlink failed");
            return -1;
        }
        fd = (is_sym ? atomic_krace(target) : krace(path, &s));
        if (fd == -1)
        {
            perror("Opening file failed");
            return -1;
        }

        if (suffix)
        {
            if (fchdir(fd) == -1)
            {
                perror("fchdir failed");
                close(fd);
                return -1;
            }
            if (close(fd) == -1)
            {
                perror("close failed");
                return -1;
            }
            path = suffix;
        }
        else
        {
            break;
        }
    }
    return fd;
}
