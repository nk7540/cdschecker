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
static bool mockfs_enabled = false;

// Function to initialize the file system
void initFileSystem()
{
    printf("initFileSystem()\n");
    fs = new FileSystem();
    fs->root = new Node();
    fs->root->metadata.st_ino = 1;
    fs->root->metadata.st_mode = S_IFDIR;
    fs->root->links[0] = new Node(); // file1.html
    fs->root->links[0]->metadata.st_ino = 2;
    fs->root->links[1] = new Node(); // passwd
    fs->root->links[1]->metadata.st_ino = 3;
    fs->current = fs->root;
    fs->inodeCounter = 1;
    fs->fdCounter = 0;

    mockfs_enabled = true;
}

void resetFileSystem()
{
    printf("resetFileSystem()\n");
    mockfs_enabled = false;
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
    if (!mockfs_enabled)
    {
        return creat(path, mode);
    }

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

    if (!mockfs_enabled)
    {
        return open(path, flags, mode);
    }

    Node *fileNode = NULL;

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     fileNode = fs->current->links[(unsigned char)path[0]];
    // }
    // else
    // {
    //     if (flags & O_CREAT)
    //     {
    //         if (my_creat(path, mode) == -1)
    //         {
    //             return -1; // Failed to create file
    //         }
    //         fileNode = fs->current->links[(unsigned char)path[0]];
    //     }
    //     else
    //     {
    //         return -1; // File does not exist
    //     }
    // }
    const char *file1 = "file1.html";
    const char *passwd = "passwd";
    fs->mtx.lock();
    if (strcmp(path, file1) == 0)
    {
        printf("open(\"file1.html\")\n");
        fileNode = fs->root->links[0];
    }
    else if (strcmp(path, passwd) == 0)
    {
        printf("open(\"passwd\")\n");
        fileNode = fs->root->links[1];
    }

    if (fileNode)
    {
        int fd = ++fs->fdCounter;
        fs->openFiles[fd] = fileNode;
        fs->mtx.unlock();
        return fd;
    }
    fs->mtx.unlock();

    va_end(ap);
    return -1; // Failed to open file
}

// POSIX-like chdir function
int my_chdir(const char *path)
{
    if (!mockfs_enabled)
    {
        return chdir(path);
    }

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
    if (!mockfs_enabled)
    {
        return link(oldpath, newpath);
    }

    fs->mtx.lock();
    printf("link(\"file1.html\")\n");
    // if (fs->current->links[(unsigned char)newpath[0]])
    // {
    //     fs->mtx.unlock();
    //     return -1; // New path already exists
    // }
    // if (fs->current->links[(unsigned char)oldpath[0]])
    // {
    //     fs->current->links[(unsigned char)newpath[0]] = fs->current->links[(unsigned char)oldpath[0]];
    //     fs->current->links[(unsigned char)oldpath[0]]->metadata.st_nlink++;
    //     fs->mtx.unlock();
    //     return 0;
    // }

    fs->mtx.unlock();
    return -1; // Old path not found
}

// POSIX-like unlink function
int my_unlink(const char *path)
{
    if (!mockfs_enabled)
    {
        return unlink(path);
    }

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

// POSIX-like stat function
int my_stat(const char *path, struct stat *buf)
{
    if (!mockfs_enabled)
    {
        return stat(path, buf);
    }

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     *buf = fs->current->links[(unsigned char)path[0]]->metadata;
    //     return 0;
    // }
    fs->mtx.lock();
    if (strcmp(path, "file1.html") == 0)
    {
        if (strcmp(fs->root->links[0]->target, "passwd") == 0)
        {
            *buf = fs->root->links[1]->metadata;
            return 0;
        }
        *buf = fs->root->links[0]->metadata;
        return 0;
    }

    fs->mtx.unlock();
    return -1; // Path not found
}

int my_lstat(const char *path, struct stat *buf)
{
    if (!mockfs_enabled)
    {
        return lstat(path, buf);
    }

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     *buf = fs->current->links[(unsigned char)path[0]]->metadata;
    //     return 0;
    // }
    fs->mtx.lock();
    if (strcmp(path, "file1.html") == 0)
    {
        *buf = fs->root->links[0]->metadata;
        return 0;
    }

    fs->mtx.unlock();
    return -1; // Path not found
}
