#include "mockfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// Global file system instance
FileSystem fs;

// Function to create a new node
Node *createNode()
{
    printf("createNode()\n");
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode)
    {
        perror("Failed to allocate node");
        exit(EXIT_FAILURE);
    }
    memset(newNode, 0, sizeof(Node));
    newNode->target = NULL;
    return newNode;
}

// Function to initialize the file system
void initFileSystem()
{
    fs.root = createNode();
    fs.root->metadata.st_ino = 1;
    fs.root->metadata.st_mode = S_IFDIR;
    fs.current = fs.root;
    fs.inodeCounter = 1;
    fs.fdCounter = 0;
    pthread_mutex_init(&fs.mtx, NULL);
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
    deleteNode(fs.root);
    pthread_mutex_destroy(&fs.mtx);
}

// POSIX-like creat function
int my_creat(const char *path, mode_t mode)
{
    pthread_mutex_lock(&fs.mtx);

    if (fs.current->links[(unsigned char)path[0]])
    {
        pthread_mutex_unlock(&fs.mtx);
        return -1; // File already exists
    }

    Node *newNode = createNode();
    newNode->metadata.st_ino = ++fs.inodeCounter;
    newNode->metadata.st_mode = mode | S_IFREG;
    newNode->parent = fs.current;

    fs.current->links[(unsigned char)path[0]] = newNode;
    fs.current->metadata.st_nlink++;

    pthread_mutex_unlock(&fs.mtx);
    return open(path, O_WRONLY);
}

// POSIX-like open function
int my_open(const char *path, int flags, mode_t mode)
{
    pthread_mutex_lock(&fs.mtx);

    Node *fileNode = NULL;

    if (fs.current->links[(unsigned char)path[0]])
    {
        fileNode = fs.current->links[(unsigned char)path[0]];
    }
    else
    {
        if (flags & O_CREAT)
        {
            if (my_creat(path, mode) == -1)
            {
                pthread_mutex_unlock(&fs.mtx);
                return -1; // Failed to create file
            }
            fileNode = fs.current->links[(unsigned char)path[0]];
        }
        else
        {
            pthread_mutex_unlock(&fs.mtx);
            return -1; // File does not exist
        }
    }

    if (fileNode)
    {
        int fd = ++fs.fdCounter;
        fs.openFiles[fd] = fileNode;
        pthread_mutex_unlock(&fs.mtx);
        return fd;
    }

    pthread_mutex_unlock(&fs.mtx);
    return -1; // Failed to open file
}

// POSIX-like chdir function
int my_chdir(const char *path)
{
    pthread_mutex_lock(&fs.mtx);

    if (fs.current->links[(unsigned char)path[0]] &&
        (fs.current->links[(unsigned char)path[0]]->metadata.st_mode & S_IFDIR))
    {
        fs.current = fs.current->links[(unsigned char)path[0]];
        pthread_mutex_unlock(&fs.mtx);
        return 0;
    }

    pthread_mutex_unlock(&fs.mtx);
    return -1; // Path not found or not a directory
}

// POSIX-like link function
int my_link(const char *oldpath, const char *newpath)
{
    pthread_mutex_lock(&fs.mtx);

    if (fs.current->links[(unsigned char)newpath[0]])
    {
        pthread_mutex_unlock(&fs.mtx);
        return -1; // New path already exists
    }
    if (fs.current->links[(unsigned char)oldpath[0]])
    {
        fs.current->links[(unsigned char)newpath[0]] = fs.current->links[(unsigned char)oldpath[0]];
        fs.current->links[(unsigned char)oldpath[0]]->metadata.st_nlink++;
        pthread_mutex_unlock(&fs.mtx);
        return 0;
    }

    pthread_mutex_unlock(&fs.mtx);
    return -1; // Old path not found
}

// POSIX-like unlink function
int my_unlink(const char *path)
{
    pthread_mutex_lock(&fs.mtx);

    if (fs.current->links[(unsigned char)path[0]])
    {
        Node *node = fs.current->links[(unsigned char)path[0]];
        node->metadata.st_nlink--;

        if (node->metadata.st_nlink == 0)
        {
            deleteNode(node);
        }

        fs.current->links[(unsigned char)path[0]] = NULL;
        pthread_mutex_unlock(&fs.mtx);
        return 0;
    }

    pthread_mutex_unlock(&fs.mtx);
    return -1; // Path not found
}

// POSIX-like symlink function
int my_symlink(const char *target, const char *linkpath)
{
    pthread_mutex_lock(&fs.mtx);

    if (fs.current->links[(unsigned char)linkpath[0]])
    {
        pthread_mutex_unlock(&fs.mtx);
        return -1; // Link path already exists
    }

    Node *newNode = createNode();
    newNode->metadata.st_ino = ++fs.inodeCounter;
    newNode->metadata.st_mode = S_IFLNK;
    newNode->target = strdup(target);
    newNode->parent = fs.current;

    fs.current->links[(unsigned char)linkpath[0]] = newNode;
    fs.current->metadata.st_nlink++;

    pthread_mutex_unlock(&fs.mtx);
    return 0;
}

// POSIX-like stat function
int my_stat(const char *path, struct stat *buf)
{
    pthread_mutex_lock(&fs.mtx);

    if (fs.current->links[(unsigned char)path[0]])
    {
        *buf = fs.current->links[(unsigned char)path[0]]->metadata;
        pthread_mutex_unlock(&fs.mtx);
        return 0;
    }

    pthread_mutex_unlock(&fs.mtx);
    return -1; // Path not found
}
