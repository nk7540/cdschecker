#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

// Node structure representing an inode
typedef struct Node
{
    struct stat metadata;
    struct Node *parent;
    struct Node *links[256];
    char *target; // for symlink target path
} Node;

// FileSystem structure to hold the global file system state
typedef struct FileSystem
{
    Node *root;
    Node *current;
    ino_t inodeCounter;
    int fdCounter;
    Node *openFiles[256];
    pthread_mutex_t mtx;
} FileSystem;

// Global file system instance
extern FileSystem fs;

// Function to create a new node
Node *createNode();

// Function to initialize the file system
void initFileSystem();

// Function to delete a node recursively
void deleteNode(Node *node);

// Function to destroy the file system
void destroyFileSystem();

// POSIX-like creat function
int my_creat(const char *path, mode_t mode);

// POSIX-like open function
int my_open(const char *path, int flags, mode_t mode);

// POSIX-like chdir function
int my_chdir(const char *path);

// POSIX-like link function
int my_link(const char *oldpath, const char *newpath);

// POSIX-like unlink function
int my_unlink(const char *path);

// POSIX-like symlink function
int my_symlink(const char *target, const char *linkpath);

// POSIX-like stat function
int my_stat(const char *path, struct stat *buf);

#endif // FILE_SYSTEM_H
