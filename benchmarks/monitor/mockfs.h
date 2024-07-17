#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include "mutex"

// Structure to represent each node (inode)
struct Node
{
    struct stat metadata;
    std::unordered_map<std::string, Node *> links; // Links to child nodes
    Node *parent;                                  // Link to the parent node

    Node() : parent(nullptr)
    {
        // Initialize metadata to zero
        memset(&metadata, 0, sizeof(metadata));
    }
};

// Class to represent the file system
class FileSystem
{
public:
    FileSystem()
    {
        root = new Node();
        root->metadata.st_ino = 1; // Assign inode number for root
        current = root;
    }

    ~FileSystem()
    {
        deleteNode(root);
    }

    void createFile(const std::string &name, bool isDirectory)
    {
        Node *newNode = new Node();
        newNode->metadata.st_ino = ++inodeCounter;
        newNode->metadata.st_mode = isDirectory ? S_IFDIR : S_IFREG;
        newNode->parent = current;

        // Set link from current node to new node
        current->links[name] = newNode;

        // Set parent link
        newNode->links[".."] = current;
        current->metadata.st_nlink++; // Increment link count for the current node
    }

    void changeDirectory(const std::string &path)
    {
        if (current->links.find(path) != current->links.end())
        {
            current = current->links[path];
        }
        else
        {
            std::cerr << "Path not found\n";
        }
    }

    void printCurrentPath()
    {
        std::vector<std::string> path;
        Node *temp = current;
        while (temp != root)
        {
            for (const auto &link : temp->parent->links)
            {
                if (link.second == temp && link.first != "..")
                {
                    path.push_back(link.first);
                    break;
                }
            }
            temp = temp->parent;
        }
        std::cout << "/";
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            std::cout << *it << "/";
        }
        std::cout << "\n";
    }

private:
    Node *root;
    Node *current;
    ino_t inodeCounter = 1;

    void deleteNode(Node *node)
    {
        for (auto &link : node->links)
        {
            if (link.first != "..")
            {
                deleteNode(link.second);
            }
        }
        delete node;
    }
};
