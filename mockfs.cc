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
unordered_map<ino_t, Node *> inode_map;
int inode_count = 0;
static bool mockfs_enabled = false;
unordered_map<int, Node *> sys_open_files[2];
unordered_map<int, Node *> open_files;

mutex *mtx;

// Function to initialize the file system
void initFileSystem()
{
    printf("initFileSystem()\n");

    mtx = new mutex();
    // Node *root = allocate();
    // Node *file1 = allocate();
    // Node *file2 = allocate();
    // link_node(root, file1, "file1");
    // link_node(root, file2, "file2");

    mockfs_enabled = true;
}

void resetFileSystem()
{
    printf("resetFileSystem()\n");
    mockfs_enabled = false;
    free(mtx);
}

void switchProc(int pid)
{
    open_files = sys_open_files[pid];
}

Node *allocate()
{
    Node *node;

    node->metadata.st_ino = inode_count;
    inode_map[inode_count] = node;

    inode_count++;
    return node;
}

void link_node(Node *src, Node *dst, string name)
{
    src->links.push_back({dst->metadata.st_ino, name});
};

void unlink_node(Node *src, string name)
{
    for (int i = 0; i < src->links.size(); i++)
    {
        if (src->links[i].second == name)
        {
            free(inode_map[src->links[i].first]);
            src->links.erase(src->links.begin() + i);
        }
    }
};

// Function to destroy the file system
void destroyFileSystem()
{
    for (auto &[ino, node] : inode_map)
    {
        free(node);
    }
}

void traversePath(const string &pathname)
{
    // Start traversal from the root node
    Node *currentNode = inode_map[0];

    size_t pos = 1;
    while (pos < pathname.size())
    {
        size_t nextSlash = pathname.find('/', pos);
        string component = pathname.substr(pos, nextSlash - pos);

        // Find the next node to traverse
        bool found = false;
        for (auto &link : currentNode->links)
        {
            if (component == link.second)
            {
                found = true;
                currentNode = inode_map[link.first];

                // Handle symbolic links (symlink)
                if (currentNode->target)
                {
                    string symlinkTarget(currentNode->target);
                    cout << "Following symlink: " << symlinkTarget << endl;
                    traversePath(symlinkTarget + pathname.substr(nextSlash));
                    return;
                }
                break;
            }
        }

        if (!found)
        {
            cout << "Path not found: " << pathname << endl;
            return;
        }

        if (nextSlash == string::npos)
            break;
        pos = nextSlash + 1;
    }

    cout << "Traversal successful. Reached inode: " << currentNode->metadata.st_ino << endl;
}

// POSIX-like creat function
int my_creat(const char *path, mode_t mode)
{
    if (!mockfs_enabled)
    {
        return creat(path, mode);
    }

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     return -1; // File already exists
    // }

    // Node *newNode = new Node();
    // newNode->metadata.st_ino = ++fs->inodeCounter;
    // newNode->metadata.st_mode = mode | S_IFREG;
    // newNode->parent = fs->current;

    // fs->current->links[(unsigned char)path[0]] = newNode;
    // fs->current->metadata.st_nlink++;

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

    mtx->lock();
    printf("open(\"file1.html\")\n");
    mtx->unlock();

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

    // if (fs->current->links[(unsigned char)path[0]] &&
    //     (fs->current->links[(unsigned char)path[0]]->metadata.st_mode & S_IFDIR))
    // {
    //     fs->current = fs->current->links[(unsigned char)path[0]];
    //     return 0;
    // }

    return -1; // Path not found or not a directory
}

// POSIX-like link function
int my_symlink(const char *oldpath, const char *newpath)
{
    if (!mockfs_enabled)
    {
        return symlink(oldpath, newpath);
    }

    mtx->lock();
    printf("symlink(\"file1.html\")\n");
    mtx->unlock();
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

    return -1; // Old path not found
}

// POSIX-like unlink function
int my_unlink(const char *path)
{
    if (!mockfs_enabled)
    {
        return unlink(path);
    }

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     Node *node = fs->current->links[(unsigned char)path[0]];
    //     node->metadata.st_nlink--;

    //     if (node->metadata.st_nlink == 0)
    //     {
    //         deleteNode(node);
    //     }

    //     fs->current->links[(unsigned char)path[0]] = NULL;
    //     return 0;
    // }

    return -1; // Path not found
}

// POSIX-like stat function
int my_stat(const char *path, struct stat *buf)
{
    if (!mockfs_enabled)
    {
        return stat(path, buf);
    }
    return stat(path, buf);

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     *buf = fs->current->links[(unsigned char)path[0]]->metadata;
    //     return 0;
    // }
    printf("my_stat(%s, buf)\n", path);
    // if (strcmp(path, "file1.html") == 0)
    // {
    //     if (strcmp(fs->root->links[0]->target, "passwd") == 0)
    //     {
    //         *buf = fs->root->links[1]->metadata;
    //         fs->mtx.unlock();
    //         return 0;
    //     }
    //     *buf = fs->root->links[0]->metadata;
    //     fs->mtx.unlock();
    //     return 0;
    // }
    errno = ENOENT;

    return -1; // Path not found
}

int my_lstat(const char *path, struct stat *buf)
{
    if (!mockfs_enabled)
    {
        return lstat(path, buf);
    }
    return lstat(path, buf);

    // if (fs->current->links[(unsigned char)path[0]])
    // {
    //     *buf = fs->current->links[(unsigned char)path[0]]->metadata;
    //     return 0;
    // }
    printf("my_lstat(%s, buf)\n", path);
    // if (strcmp(path, "file1.html") == 0)
    // {
    //     *buf = fs->root->links[0]->metadata;
    //     fs->mtx.unlock();
    //     return 0;
    // }
    errno = ENOENT;

    return -1; // Path not found
}
