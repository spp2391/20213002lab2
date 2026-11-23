#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
static void walk(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) { perror(path); return; }
    printf("\n%s:\n", path);
    struct dirent *e;
    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        printf("%s  ", e->d_name);
    }
    puts(""); rewinddir(dir);
    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char child[PATH_MAX];
        int n = snprintf(child, sizeof(child), "%s/%s", path, e->d_name);
        if (n < 0 || (size_t)n >= sizeof(child)) continue;
        struct stat st;
        if (lstat(child, &st) == 0 && S_ISDIR(st.st_mode)) walk(child);
    }
    closedir(dir);
}
int main(int argc, char **argv) { walk(argc > 1 ? argv[1] : "."); return 0; }
