#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
int main(void) {
    if (mkdir("demo_dir", 0755) == -1 && errno != EEXIST) { perror("mkdir"); return 1; }
    int fd = open("demo_dir/hello.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return 1; }
    const char *msg = "Hello Linux file I/O!\n";
    if (write(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) { perror("write"); close(fd); return 1; }
    close(fd);
    fd = open("demo_dir/hello.txt", O_RDONLY);
    if (fd < 0) { perror("open read"); return 1; }
    char buf[128]; ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) { perror("read"); close(fd); return 1; }
    buf[n] = '\0'; printf("read: %s", buf); close(fd);
    struct stat st;
    if (stat("demo_dir/hello.txt", &st) == 0) printf("size: %lld bytes\n", (long long)st.st_size);
    return 0;
}
