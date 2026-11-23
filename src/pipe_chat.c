/* 실습 8: FIFO(named pipe) 두 개로 독립된 터미널 A/B 채팅 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>

#define ATOB "/tmp/lab2_pipe_a_to_b"
#define BTOA "/tmp/lab2_pipe_b_to_a"
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--clean") == 0) {
        unlink(ATOB); unlink(BTOA); puts("FIFO 파일 삭제"); return 0;
    }
    if (argc != 2 || (strcmp(argv[1], "a") && strcmp(argv[1], "b"))) {
        fprintf(stderr, "사용법: %s a|b|--clean\n", argv[0]); return 1;
    }
    int a = strcmp(argv[1], "a") == 0;
    if (mkfifo(ATOB, 0600) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }
    if (mkfifo(BTOA, 0600) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }
    const char *outpath = a ? ATOB : BTOA;
    const char *inpath = a ? BTOA : ATOB;
    // O_RDWR는 양방향 연결 대기에서 발생할 교착을 피하기 위해 사용(Linux FIFO 방식)
    int out = open(outpath, O_RDWR | O_NONBLOCK);
    int in  = open(inpath, O_RDWR | O_NONBLOCK);
    if (out < 0 || in < 0) { perror("open"); return 1; }
    printf("%c 채팅 시작 (/quit 종료)\n", a ? 'A' : 'B');
    char line[256], buf[256];
    while (1) {
        fd_set set; FD_ZERO(&set); FD_SET(STDIN_FILENO, &set); FD_SET(in, &set);
        int maxfd = in > STDIN_FILENO ? in : STDIN_FILENO;
        int rc = select(maxfd + 1, &set, NULL, NULL, NULL);
        if (rc < 0) { if (errno == EINTR) continue; perror("select"); break; }
        if (FD_ISSET(STDIN_FILENO, &set)) {
            if (!fgets(line, sizeof(line), stdin)) break;
            if (write(out, line, strlen(line)) < 0) { perror("write"); break; }
            if (strncmp(line, "/quit", 5) == 0) break;
        }
        if (FD_ISSET(in, &set)) {
            ssize_t n = read(in, buf, sizeof(buf)-1);
            if (n > 0) {
                buf[n] = '\0'; printf("[상대] %s", buf); fflush(stdout);
                if (strncmp(buf, "/quit", 5) == 0) break;
            }
        }
    }
    close(in); close(out); return 0;
}
