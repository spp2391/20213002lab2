#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

static int copy_stream(int src, int dst) {
    char buf[4096]; ssize_t n;
    while ((n = read(src, buf, sizeof(buf))) > 0) {
        ssize_t pos = 0;
        while (pos < n) {
            ssize_t k = write(dst, buf + pos, (size_t)(n - pos));
            if (k < 0 && errno == EINTR) continue;
            if (k <= 0) { perror("write"); return 1; }
            pos += k;
        }
    }
    if (n < 0) { perror("read"); return 1; }
    return 0;
}
static int cp_one(const char *from, const char *to) {
    struct stat sa, sb;
    if (stat(from, &sa) < 0) { perror("cp source"); return 1; }
    if (!S_ISREG(sa.st_mode)) { fprintf(stderr,"cp: 일반 파일만 지원\n"); return 1; }
    if (stat(to, &sb) == 0 && sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino) {
        fprintf(stderr,"cp: 같은 파일입니다\n"); return 1;
    }
    int src = open(from, O_RDONLY);
    if (src < 0) { perror("cp open source"); return 1; }
    int dst = open(to, O_WRONLY|O_CREAT|O_TRUNC, sa.st_mode & 0777);
    if (dst < 0) { perror("cp open dest"); close(src); return 1; }
    int ret = copy_stream(src, dst); close(src); if (close(dst) < 0) ret = 1; return ret;
}
static int ls_one(const char *path) {
    DIR *d = opendir(path); if (!d) { perror("ls"); return 1; }
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] != '.') printf("%s  ", e->d_name);
    }
    putchar('\n'); closedir(d); return 0;
}
static int cat_one(const char *name) {
    int fd = name ? open(name, O_RDONLY) : STDIN_FILENO;
    if (fd < 0) { perror("cat"); return 1; }
    int ret = copy_stream(fd, STDOUT_FILENO);
    if (name) close(fd);
    return ret;
}
static int grep_one(const char *needle, const char *path) {
    FILE *fp = path ? fopen(path, "r") : stdin;
    if (!fp) { perror("grep"); return 1; }
    char *line = NULL; size_t cap = 0; ssize_t n; int found = 0;
    while ((n = getline(&line, &cap, fp)) >= 0) {
        (void)n;
        if (strstr(line, needle)) { fputs(line, stdout); found = 1; }
    }
    free(line); if (path) fclose(fp);
    return found ? 0 : 1;
}
int run_builtin(char *const argv[]) {
    if (!argv || !argv[0]) return -1;
    const char *cmd = argv[0];
    int n = 0; while (argv[n]) n++;
    if (!strcmp(cmd,"pwd")) {
        char *p = getcwd(NULL,0); if (!p) {perror("pwd"); return 1;}
        puts(p); free(p); return 0;
    }
    if (!strcmp(cmd,"ls")) return ls_one(n>1?argv[1]:".");
    if (!strcmp(cmd,"mkdir")) {if(n!=2) goto usage; if(mkdir(argv[1],0755)<0){perror(cmd); return 1;} return 0;}
    if (!strcmp(cmd,"rmdir")) {if(n!=2) goto usage; if(rmdir(argv[1])<0){perror(cmd); return 1;} return 0;}
    if (!strcmp(cmd,"rm")) {if(n!=2) goto usage; if(unlink(argv[1])<0){perror(cmd); return 1;} return 0;}
    if (!strcmp(cmd,"ln")) {if(n!=3) goto usage; if(link(argv[1],argv[2])<0){perror(cmd); return 1;} return 0;}
    if (!strcmp(cmd,"cp")) {if(n!=3) goto usage; return cp_one(argv[1],argv[2]);}
    if (!strcmp(cmd,"mv")) {
        if(n!=3) goto usage;
        if(rename(argv[1],argv[2])==0) return 0;
        if(errno!=EXDEV){perror(cmd);return 1;}
        if(cp_one(argv[1],argv[2])) return 1;
        if(unlink(argv[1])<0){perror("mv unlink");return 1;}return 0;
    }
    if (!strcmp(cmd,"cat")) {
        if(n>2) goto usage;
        return cat_one(n==2?argv[1]:NULL);
    }
    if (!strcmp(cmd,"grep")) {
        if(n<2 || n>3) goto usage;
        return grep_one(argv[1],n==3?argv[2]:NULL);
    }
    return -1;
usage:
    fprintf(stderr,"%s: 인자 개수를 확인하세요\n",cmd);return 1;
}

