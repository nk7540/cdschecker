#ifndef __MOCKFS_H__
#define __MOCKFS_H__

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sys/stat.h> // For stat metadata
#include <fcntl.h>    // For open flags
#include <map>

// Forward declaration of INode class
class INode;

// Type alias for a raw pointer to INode
using INodePtr = INode *;
using FileDescriptor = int;

class INode
{
public:
    struct stat metadata;                      // Metadata for the node
    std::unordered_set<std::string> hardLinks; // Names pointing to this node
    std::string symlinkTarget;                 // Target of a symbolic link (empty if not a symlink)

    // Constructor
    INode(const struct stat &nodeMetadata);

    // Add a hard link
    void addHardLink(const std::string &name);

    // Remove a hard link
    void removeHardLink(const std::string &name);

    // Display the node's details
    void displayLinks() const;

    ~INode();
};

class FileSystem
{
private:
    std::unordered_map<std::string, INodePtr> nodes;    // Map to store nodes by names
    std::map<FileDescriptor, INodePtr> fileDescriptors; // Map of open file descriptors
    FileDescriptor nextFd = 3;                          // File descriptor counter

public:
    FileSystem();
    ~FileSystem();

    // Create a symbolic link
    int symlink(const std::string &oldpath, const std::string &newpath);

    // Retrieve metadata of a file or directory
    int stat(const std::string &path, struct stat *buf);

    // Open a file
    int open(const std::string &path, int flags);

    // Unlink a file
    int unlink(const std::string &path);
};

extern "C"
{
    INode *allocate();

    // Function to create a new node
    INode *createINode();

    // Function to initialize the file system
    void initFileSystem();

    // Function to reset the file system
    void resetFileSystem();

    // Function to delete a node recursively
    void deleteINode(INode *node);

    // Function to destroy the file system
    void destroyFileSystem();

    // POSIX-like creat function
    int my_creat(const char *path, mode_t mode);

    // POSIX-like open function
    int my_open(const char *path, int flags, ...);

    // POSIX-like chdir function
    int my_chdir(const char *path);

    // POSIX-like link function
    int my_symlink(const char *oldpath, const char *newpath);

    // POSIX-like unlink function
    int my_unlink(const char *path);

    // POSIX-like stat function
    int my_stat(const char *path, struct stat *buf);

    // POSIX-like lstat function
    int my_lstat(const char *path, struct stat *buf);

    // Functions for attackers
    int unlink_and_symlink(const char *oldpath, const char *newpath);
}

#endif // FILE_SYSTEM_H
