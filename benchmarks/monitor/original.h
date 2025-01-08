int unsafe_open(const char *path, int flags, ...);
int safe_open(const char *path, int flags, char **final_resolved_path, ...);
int krace(const char *path, struct stat *s);
int atomic_krace(char *path);
