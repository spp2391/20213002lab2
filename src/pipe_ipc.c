#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    int fds[2]; if (pipe(fds) < 0) { perror("pipe"); return 1; }
    pid_t pid = fork(); if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        close(fds[1]); char buf[128]; ssize_t n = read(fds[0], buf, sizeof(buf)-1);
        if (n > 0) { buf[n] = 0; printf("자식이 받은 메시지: %s\n", buf); }
        close(fds[0]); _exit(n < 0 ? 1 : (fflush(stdout), 0));
    }
    close(fds[0]); const char msg[] = "안녕하세요, 부모 프로세스입니다.";
    if (write(fds[1], msg, sizeof(msg)) < 0) perror("write");
    close(fds[1]); waitpid(pid, NULL, 0); return 0;
}
