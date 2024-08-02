#ifndef __MOCKFS_H__
#define __MOCKFS_H__

#include <sys/stat.h>
#include <pthread.h>
#include "mutex"

using namespace std;

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
    mutex mtx;
} FileSystem;

extern "C"
{
    // Function to create a new node
    Node *createNode();

    // Function to initialize the file system
    void initFileSystem();

    // Function to reset the file system
    void resetFileSystem();

    // Function to delete a node recursively
    void deleteNode(Node *node);

    // Function to destroy the file system
    void destroyFileSystem();

    // POSIX-like creat function
    int my_creat(const char *path, mode_t mode);

    // POSIX-like open function
    int my_open(const char *path, int flags, ...);

    // POSIX-like chdir function
    int my_chdir(const char *path);

    // POSIX-like link function
    int my_link(const char *oldpath, const char *newpath);

    // POSIX-like unlink function
    int my_unlink(const char *path);

    // POSIX-like stat function
    int my_stat(const char *path, struct stat *buf);

    // POSIX-like lstat function
    int my_lstat(const char *path, struct stat *buf);
}

#endif // FILE_SYSTEM_H
