#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

typedef int FileDescriptor;

typedef enum FileType {
    Regular,
    Directory,
    Symlink
} FileType;

typedef struct {
    FileType type;
    char *content;
    struct HashMap *children;
} FileTypeData;

typedef struct HashMapNode {
    char *key;
    FileTypeData *value;
    struct HashMapNode *next;
} HashMapNode;

typedef struct HashMap {
    HashMapNode **buckets;
    size_t capacity;
} HashMap;

typedef struct {
    pthread_rwlock_t lock;
    HashMap *tree;
} FS_Tree;

FS_Tree fs_tree;

pthread_key_t next_fd_key;
pthread_key_t open_files_key;
pthread_key_t current_dir_key;

FileDescriptor get_next_fd() {
    FileDescriptor *next_fd = (FileDescriptor *)pthread_getspecific(next_fd_key);
    if (!next_fd) {
        next_fd = (FileDescriptor *)malloc(sizeof(FileDescriptor));
        *next_fd = 3;
        pthread_setspecific(next_fd_key, next_fd);
    }
    return (*next_fd)++;
}

HashMap *create_hashmap(size_t capacity) {
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    map->capacity = capacity;
    map->buckets = (HashMapNode **)calloc(capacity, sizeof(HashMapNode *));
    return map;
}

FileTypeData *create_filetype_data(FileType type, const char *content) {
    FileTypeData *data = (FileTypeData *)malloc(sizeof(FileTypeData));
    data->type = type;
    data->content = content ? strdup(content) : NULL;
    data->children = type == Directory ? create_hashmap(16) : NULL;
    return data;
}

void hashmap_put(HashMap *map, const char *key, FileTypeData *value) {
    size_t idx = hash(key) % map->capacity;
    HashMapNode *new_node = (HashMapNode *)malloc(sizeof(HashMapNode));
    new_node->key = strdup(key);
    new_node->value = value;
    new_node->next = map->buckets[idx];
    map->buckets[idx] = new_node;
}

FileTypeData *hashmap_get(HashMap *map, const char *key) {
    size_t idx = hash(key) % map->capacity;
    HashMapNode *node = map->buckets[idx];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

size_t hash(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void initialize_mockfs() {
    pthread_rwlock_wrlock(&fs_tree.lock);
    hashmap_put(fs_tree.tree, "/var/www/assets/noncredential", create_filetype_data(Regular, "noncredential content"));
    hashmap_put(fs_tree.tree, "/var/www/assets/credentials", create_filetype_data(Regular, "credentials content"));
    hashmap_put(fs_tree.tree, "/var/www/assets/symlink", create_filetype_data(Symlink, "assets/noncredential"));
    pthread_rwlock_unlock(&fs_tree.lock);
}

void register_fd_in_proc(const char *resolved_path, FileDescriptor fd) {
    // Implementation here
}

int openat(int dirfd, const char *pathname, int flags) {
    char path[PATH_MAX];
    strncpy(path, pathname, PATH_MAX);

    // Lock the filesystem tree
    pthread_rwlock_rdlock(&fs_tree.lock);

    char *base_path;
    if (path[0] == '/') {
        base_path = "";
    } else if (dirfd == AT_FDCWD) {
        base_path = (char *)pthread_getspecific(current_dir_key);
    } else {
        // Get directory path by file descriptor from open_files
        HashMap *open_files = (HashMap *)pthread_getspecific(open_files_key);
        FileTypeData *dir_data = hashmap_get(open_files, &dirfd);
        if (!dir_data) {
            pthread_rwlock_unlock(&fs_tree.lock);
            return -1;
        }
        base_path = dir_data->content;
    }

    // Construct the full path
    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", base_path, path);
    char *resolved_path = realpath(full_path, NULL);

    if (!resolved_path) {
        pthread_rwlock_unlock(&fs_tree.lock);
        return -1;
    }

    // Check if the path exists in the filesystem
    FileTypeData *data = hashmap_get(fs_tree.tree, resolved_path);
    if (!data) {
        pthread_rwlock_unlock(&fs_tree.lock);
        return -1;
    }

    // Unlock the filesystem tree
    pthread_rwlock_unlock(&fs_tree.lock);

    // Get the next available file descriptor
    FileDescriptor fd = get_next_fd();

    // Register the file descriptor and the path
    register_fd_in_proc(resolved_path, fd);

    // Add to open_files
    HashMap *open_files = (HashMap *)pthread_getspecific(open_files_key);
    hashmap_put(open_files, fd, data);

    return fd;
}

int open(const char *pathname, int flags) {
    return openat(AT_FDCWD, pathname, flags);
}

int main() {
    pthread_key_create(&next_fd_key, free);
    pthread_key_create(&open_files_key, free);
    pthread_key_create(&current_dir_key, free);

    fs_tree.tree = create_hashmap(128);
    pthread_rwlock_init(&fs_tree.lock, NULL);

    initialize_mockfs();

    const char *path = "/var/www/assets/noncredential";
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        printf("Opened file %s with file descriptor %d\n", path, fd);
    } else {
        printf("Failed to open file %s\n", path);
    }

    return 0;
}
