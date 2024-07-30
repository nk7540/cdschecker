#include "mockfs.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Global file system instance
FileSystem *fs;

// Function to initialize the file system
void initFileSystem()
{
    printf("initFileSystem()\n");
    fs = new FileSystem();
    fs->root = new Node();
    fs->root->metadata.st_ino = 1;
    fs->root->metadata.st_mode = S_IFDIR;
    fs->current = fs->root;
    fs->inodeCounter = 1;
    fs->fdCounter = 0;
}

// Function to delete a node recursively
void deleteNode(Node *node)
{
    for (int i = 0; i < 256; ++i)
    {
        if (node->links[i])
        {
            deleteNode(node->links[i]);
        }
    }
    free(node->target);
    free(node);
}

// Function to destroy the file system
void destroyFileSystem()
{
    deleteNode(fs->root);
}

// POSIX-like creat function
int my_creat(const char *path, mode_t mode)
{
    if (fs->current->links[(unsigned char)path[0]])
    {
        return -1; // File already exists
    }

    Node *newNode = new Node();
    newNode->metadata.st_ino = ++fs->inodeCounter;
    newNode->metadata.st_mode = mode | S_IFREG;
    newNode->parent = fs->current;

    fs->current->links[(unsigned char)path[0]] = newNode;
    fs->current->metadata.st_nlink++;

    return open(path, O_WRONLY);
}

// POSIX-like open function
int my_open(const char *path, int flags, ...)
{
    va_list ap;
    va_start(ap, flags);
    mode_t mode = va_arg(ap, mode_t);

    Node *fileNode = NULL;

    if (fs->current->links[(unsigned char)path[0]])
    {
        fileNode = fs->current->links[(unsigned char)path[0]];
    }
    else
    {
        if (flags & O_CREAT)
        {
            if (my_creat(path, mode) == -1)
            {
                return -1; // Failed to create file
            }
            fileNode = fs->current->links[(unsigned char)path[0]];
        }
        else
        {
            return -1; // File does not exist
        }
    }

    if (fileNode)
    {
        int fd = ++fs->fdCounter;
        fs->openFiles[fd] = fileNode;
        return fd;
    }

    va_end(ap);
    return -1; // Failed to open file
}

// POSIX-like chdir function
int my_chdir(const char *path)
{
    if (fs->current->links[(unsigned char)path[0]] &&
        (fs->current->links[(unsigned char)path[0]]->metadata.st_mode & S_IFDIR))
    {
        fs->current = fs->current->links[(unsigned char)path[0]];
        return 0;
    }

    return -1; // Path not found or not a directory
}

// POSIX-like link function
int my_link(const char *oldpath, const char *newpath)
{
    if (fs->current->links[(unsigned char)newpath[0]])
    {
        return -1; // New path already exists
    }
    if (fs->current->links[(unsigned char)oldpath[0]])
    {
        fs->current->links[(unsigned char)newpath[0]] = fs->current->links[(unsigned char)oldpath[0]];
        fs->current->links[(unsigned char)oldpath[0]]->metadata.st_nlink++;
        return 0;
    }

    return -1; // Old path not found
}

// POSIX-like unlink function
int my_unlink(const char *path)
{
    if (fs->current->links[(unsigned char)path[0]])
    {
        Node *node = fs->current->links[(unsigned char)path[0]];
        node->metadata.st_nlink--;

        if (node->metadata.st_nlink == 0)
        {
            deleteNode(node);
        }

        fs->current->links[(unsigned char)path[0]] = NULL;
        return 0;
    }

    return -1; // Path not found
}

// POSIX-like symlink function
int my_symlink(const char *target, const char *linkpath)
{
    if (fs->current->links[(unsigned char)linkpath[0]])
    {
        return -1; // Link path already exists
    }

    Node *newNode = new Node();
    newNode->metadata.st_ino = ++fs->inodeCounter;
    newNode->metadata.st_mode = S_IFLNK;
    newNode->target = strdup(target);
    newNode->parent = fs->current;

    fs->current->links[(unsigned char)linkpath[0]] = newNode;
    fs->current->metadata.st_nlink++;

    return 0;
}

// POSIX-like stat function
int my_stat(const char *path, struct stat *buf)
{
    if (fs->current->links[(unsigned char)path[0]])
    {
        *buf = fs->current->links[(unsigned char)path[0]]->metadata;
        return 0;
    }

    return -1; // Path not found
}
