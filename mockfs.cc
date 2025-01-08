#include "mockfs.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#include "mutex"

using namespace std;

// Global file system instance
unordered_map<ino_t, INode *> inode_map;
int inode_count = 0;
static bool mockfs_enabled = false;
unordered_map<int, INode *> sys_open_files[2];
unordered_map<int, INode *> open_files;

FileSystem *fs;
mutex *mtx;

void initFileSystem()
{
    printf("initFileSystem()\n");

    mtx = new mutex();
    fs = new FileSystem();

    mockfs_enabled = true;
}

void resetFileSystem()
{
    printf("resetFileSystem()\n");

    free(mtx);
    free(fs);

    mockfs_enabled = false;
}

// Function to destroy the file system
void destroyFileSystem()
{
    for (auto &[ino, node] : inode_map)
    {
        free(node);
    }
}

// void traversePath(const string &pathname)
// {
//     // Start traversal from the root node
//     INode *currentINode = inode_map[0];

//     size_t pos = 1;
//     while (pos < pathname.size())
//     {
//         size_t nextSlash = pathname.find('/', pos);
//         string component = pathname.substr(pos, nextSlash - pos);

//         // Find the next node to traverse
//         bool found = false;
//         for (auto &link : currentINode->links)
//         {
//             if (component == link.second)
//             {
//                 found = true;
//                 currentINode = inode_map[link.first];

//                 // Handle symbolic links (symlink)
//                 if (currentINode->target)
//                 {
//                     string symlinkTarget(currentINode->target);
//                     cout << "Following symlink: " << symlinkTarget << endl;
//                     traversePath(symlinkTarget + pathname.substr(nextSlash));
//                     return;
//                 }
//                 break;
//             }
//         }

//         if (!found)
//         {
//             cout << "Path not found: " << pathname << endl;
//             return;
//         }

//         if (nextSlash == string::npos)
//             break;
//         pos = nextSlash + 1;
//     }

//     cout << "Traversal successful. Reached inode: " << currentINode->metadata.st_ino << endl;
// }

// INode class implementation
INode::INode(const struct stat &nodeMetadata)
    : metadata(nodeMetadata), symlinkTarget("") {}

void INode::addHardLink(const std::string &name)
{
    hardLinks.insert(name);
}

void INode::removeHardLink(const std::string &name)
{
    hardLinks.erase(name);
}

void INode::displayLinks() const
{
    std::cout << "INode links:";
    for (const auto &link : hardLinks)
    {
        std::cout << " " << link;
    }
    if (!symlinkTarget.empty())
    {
        std::cout << " (symlink to " << symlinkTarget << ")";
    }
    std::cout << std::endl;
}

INode::~INode()
{
    std::cout << "INode destroyed." << std::endl;
}

// FileSystem class implementation
FileSystem::FileSystem()
{
    struct stat metadata = {};
    metadata.st_mode = S_IFDIR; // Root is a directory
    INodePtr rootINode = new INode(metadata);
    rootINode->addHardLink("/");
    nodes["/"] = rootINode;
    std::cout << "Root directory '/' created." << std::endl;
}

FileSystem::~FileSystem()
{
    for (auto &pair : nodes)
    {
        delete pair.second;
    }
    nodes.clear();
    fileDescriptors.clear();
    std::cout << "File system destroyed." << std::endl;
}

int FileSystem::symlink(const std::string &oldpath, const std::string &newpath)
{
    if (nodes.find(newpath) != nodes.end())
    {
        errno = EEXIST;
        return -1;
    }
    struct stat metadata = {};
    metadata.st_mode = S_IFLNK; // Set symbolic link mode
    INodePtr symlinkINode = new INode(metadata);
    symlinkINode->symlinkTarget = oldpath;
    nodes[newpath] = symlinkINode;
    return 0;
}

int FileSystem::stat(const std::string &path, struct stat *buf)
{
    if (nodes.find(path) == nodes.end())
    {
        errno = ENOENT;
        return -1;
    }
    *buf = nodes[path]->metadata;
    return 0;
}

int FileSystem::open(const std::string &path, int flags)
{
    if (nodes.find(path) == nodes.end())
    {
        if (flags & O_CREAT)
        {
            struct stat metadata = {};
            metadata.st_mode = S_IFREG; // Regular file
            INodePtr newINode = new INode(metadata);
            newINode->addHardLink(path);
            nodes[path] = newINode;
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }
    fileDescriptors[nextFd] = nodes[path];
    return nextFd++;
}

int FileSystem::unlink(const std::string &path)
{
    if (nodes.find(path) == nodes.end())
    {
        errno = ENOENT;
        return -1;
    }
    INodePtr targetINode = nodes[path];
    targetINode->removeHardLink(path);
    nodes.erase(path);

    if (targetINode->hardLinks.empty())
    {
        delete targetINode;
    }
    return 0;
}

extern "C"
{

    int my_open(const char *path, int flags, ...)
    {
        va_list ap;
        va_start(ap, flags);
        mode_t mode = va_arg(ap, mode_t);

        if (!mockfs_enabled)
        {
            return open(path, flags, mode);
        }
        mtx->lock();
        printf("open();\n");
        mtx->unlock();

        va_end(ap);
        return fs->open(path, flags);
    }

    int my_symlink(const char *oldpath, const char *newpath)
    {
        if (!mockfs_enabled)
        {
            return symlink(oldpath, newpath);
        }
        mtx->lock();
        printf("my_symlink();\n");
        mtx->unlock();

        return fs->symlink(oldpath, newpath);
    }

    int my_stat(const char *path, struct stat *buf)
    {
        if (!mockfs_enabled)
        {
            return stat(path, buf);
        }

        return fs->stat(path, buf);
    }

    int my_lstat(const char *path, struct stat *buf)
    {
        if (!mockfs_enabled)
        {
            return lstat(path, buf);
        }

        // TODO
        return fs->stat(path, buf);
    }

    int my_unlink(const char *path)
    {
        if (!mockfs_enabled)
        {
            return unlink(path);
        }
        mtx->lock();
        printf("my_unlink();\n");
        mtx->unlock();

        return fs->unlink(path);
    }

    int unlink_and_symlink(const char *oldpath, const char *newpath)
    {
        mtx->lock();
        printf("unlink_and_symlink();\n");
        mtx->unlock();

        // return fs->unlink(path);
        return -1;
    }
}
