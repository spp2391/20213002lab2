#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
/* 터미널 2개에서 ./mq_chat a 와 ./mq_chat b 실행 */
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--clean")) { mq_unlink("/lab2_a_to_b"); mq_unlink("/lab2_b_to_a"); puts("큐 삭제 완료"); return 0; }
    if (argc != 2 || (strcmp(argv[1], "a") && strcmp(argv[1], "b"))) { fprintf(stderr,"사용법: %s a|b\n",argv[0]); return 1; }
    int a = !strcmp(argv[1], "a");
    const char *rx_name = a ? "/lab2_b_to_a" : "/lab2_a_to_b";
    const char *tx_name = a ? "/lab2_a_to_b" : "/lab2_b_to_a";
    struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = 256, .mq_curmsgs = 0 };
    mqd_t rx = mq_open(rx_name, O_CREAT | O_RDONLY, 0600, &attr);
    mqd_t tx = mq_open(tx_name, O_CREAT | O_WRONLY, 0600, &attr);
    if (rx == (mqd_t)-1 || tx == (mqd_t)-1) { perror("mq_open"); return 1; }
    printf("%c 시작 (a가 먼저 보내고, b가 먼저 받음; /quit 종료)\n", a?'a':'b');
    char buf[256];
    while (1) {
        if (a) {
            printf("내 메시지: "); fflush(stdout); if (!fgets(buf,sizeof(buf),stdin)) break;
            if (mq_send(tx,buf,strlen(buf)+1,0)<0) { perror("mq_send"); break; }
            if (!strncmp(buf,"/quit",5)) break;
        }
        if (mq_receive(rx,buf,sizeof(buf),NULL)<0) { perror("mq_receive"); break; }
        printf("상대: %s",buf); if (!strncmp(buf,"/quit",5)) break;
        if (!a) {
            printf("내 메시지: "); fflush(stdout); if (!fgets(buf,sizeof(buf),stdin)) break;
            if (mq_send(tx,buf,strlen(buf)+1,0)<0) { perror("mq_send"); break; }
            if (!strncmp(buf,"/quit",5)) break;
        }
    }
    mq_close(rx); mq_close(tx);
    /* 두 참가자 종료 후: ./mq_chat --clean */
    return 0;
}
