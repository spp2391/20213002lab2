#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
static volatile sig_atomic_t got_signal = 0;
static void on_usr1(int signo) { (void)signo; got_signal = 1; }
int main(void) {
    struct sigaction sa = {0}; sa.sa_handler = on_usr1; sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) < 0) { perror("sigaction"); return 1; }
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) { printf("자식 PID: %ld\n", (long)getpid()); fflush(stdout); kill(getppid(), SIGUSR1); _exit(0); }
    while (!got_signal) pause();
    puts("부모: SIGUSR1 수신");
    int status; waitpid(pid, &status, 0);
    printf("자식 종료 상태: %d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 0;
}
