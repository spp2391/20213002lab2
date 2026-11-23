#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "builtin.h"

#define MAX_ARGS 128
#define MAX_CMDS 16
#define MAX_JOBS 32

typedef struct {
    char *argv[MAX_ARGS];
    char *in;
    char *out;
    int argc;
} Command;

static int terminal_ok = 0;
static pid_t shell_pgid;
static pid_t stopped_jobs[MAX_JOBS];
static int stopped_count = 0;

static void ignore_signal(int signum) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(signum, &sa, NULL);
}

static void default_signal(int signum) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(signum, &sa, NULL);
}

static int parse(char *line, Command cmd[], int *background) {
    memset(cmd, 0, sizeof(Command) * MAX_CMDS);
    *background = 0;
    int n = 1;
    char *save = NULL;
    char *token = strtok_r(line, " \t\r\n", &save);

    while (token) {
        Command *c = &cmd[n - 1];
        if (strcmp(token, "|") == 0) {
            if (c->argc == 0 || n == MAX_CMDS) return -1;
            n++;
        } else if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {
            int input = (token[0] == '<');
            char *filename = strtok_r(NULL, " \t\r\n", &save);
            if (!filename || !strcmp(filename, "|") || !strcmp(filename, "&") ||
                !strcmp(filename, "<") || !strcmp(filename, ">")) return -1;
            if (input) c->in = filename;
            else c->out = filename;
        } else if (strcmp(token, "&") == 0) {
            *background = 1;
            if (strtok_r(NULL, " \t\r\n", &save) != NULL) return -1;
            break;
        } else {
            if (c->argc >= MAX_ARGS - 1) return -1;
            c->argv[c->argc++] = token;
            c->argv[c->argc] = NULL;
        }
        token = strtok_r(NULL, " \t\r\n", &save);
    }
    if (cmd[n - 1].argc == 0) return -1;
    return n;
}

static void reap_background(void) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) { }
}

static void wait_foreground(pid_t pgid) {
    int status;
    int stopped = 0;
    pid_t result;
    while ((result = waitpid(-pgid, &status, WUNTRACED)) > 0) {
        if (WIFSTOPPED(status)) {
            stopped = 1;
            break;
        }
    }
    if (result < 0 && errno != ECHILD) perror("waitpid");

    if (terminal_ok && tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("tcsetpgrp(shell)");

    if (stopped) {
        if (stopped_count < MAX_JOBS) {
            stopped_jobs[stopped_count++] = pgid;
            printf("[정지됨] 프로세스 그룹 %ld (fg로 재개)\n", (long)pgid);
        } else {
            fprintf(stderr, "정지 작업이 너무 많아 종료합니다.\n");
            kill(-pgid, SIGCONT);
            kill(-pgid, SIGTERM);
        }
    }
}

static void run_commands(Command cmd[], int n, int background) {
    int previous_read = -1;
    pid_t pgid = 0;
    int launched = 0;
    int failed = 0;

    for (int i = 0; i < n; i++) {
        int p[2] = {-1, -1};
        if (i + 1 < n && pipe(p) < 0) { perror("pipe"); failed = 1; break; }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (p[0] >= 0) close(p[0]);
            if (p[1] >= 0) close(p[1]);
            failed = 1;
            break;
        }
        if (pid == 0) {
            /* 파이프의 모든 자식은 하나의 프로세스 그룹에 속한다. */
            setpgid(0, pgid == 0 ? 0 : pgid);
            default_signal(SIGINT);
            default_signal(SIGQUIT);
            default_signal(SIGTSTP);
            default_signal(SIGTTIN);
            default_signal(SIGTTOU);
            default_signal(SIGCHLD);

            if (previous_read >= 0 && dup2(previous_read, STDIN_FILENO) < 0) {
                perror("dup2(stdin)"); _exit(1);
            }
            if (p[1] >= 0 && dup2(p[1], STDOUT_FILENO) < 0) {
                perror("dup2(stdout)"); _exit(1);
            }
            if (previous_read >= 0) close(previous_read);
            if (p[0] >= 0) close(p[0]);
            if (p[1] >= 0) close(p[1]);

            if (background && i == 0 && cmd[i].in == NULL) {
                int devnull = open("/dev/null", O_RDONLY);
                if (devnull >= 0) { dup2(devnull, STDIN_FILENO); close(devnull); }
            }
            if (cmd[i].in) {
                int fd = open(cmd[i].in, O_RDONLY);
                if (fd < 0) { perror("<"); _exit(1); }
                if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2"); _exit(1); }
                close(fd);
            }
            if (cmd[i].out) {
                int fd = open(cmd[i].out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror(">"); _exit(1); }
                if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); _exit(1); }
                close(fd);
            }

            int rc = run_builtin(cmd[i].argv);
            if (rc >= 0) {
                fflush(stdout);
                fflush(stderr);
                _exit(rc);
            }
            execvp(cmd[i].argv[0], cmd[i].argv);
            perror(cmd[i].argv[0]);
            _exit(127);
        }
        if (pgid == 0) pgid = pid;
        if (setpgid(pid, pgid) < 0 && errno != EACCES) perror("setpgid");
        launched++;
        if (previous_read >= 0) close(previous_read);
        if (p[1] >= 0) close(p[1]);
        previous_read = p[0];
    }
    if (previous_read >= 0) close(previous_read);
    if (!launched) return;
    if (failed) {
        kill(-pgid, SIGTERM);
        while (waitpid(-pgid, NULL, 0) > 0) { }
        return;
    }
    if (background) {
        printf("[백그라운드] PID %ld\n", (long)pgid);
        return;
    }
    if (terminal_ok && tcsetpgrp(STDIN_FILENO, pgid) < 0)
        perror("tcsetpgrp(job)");
    wait_foreground(pgid);
}

int main(void) {
    shell_pgid = getpgrp();
    terminal_ok = isatty(STDIN_FILENO);
    ignore_signal(SIGINT);
    ignore_signal(SIGQUIT);
    ignore_signal(SIGTSTP);
    ignore_signal(SIGTTOU);
    ignore_signal(SIGTTIN);
    char *line = NULL;
    size_t capacity = 0;
    Command cmd[MAX_CMDS];

    for (;;) {
        reap_background();
        printf("lab2$ ");
        fflush(stdout);
        if (getline(&line, &capacity, stdin) < 0) {
            if (feof(stdin)) puts("");
            break;
        }
        int background;
        int n = parse(line, cmd, &background);
        if (n < 0) {
            fprintf(stderr, "문법 오류: | < > & 앞뒤에 공백을 입력하세요.\n");
            continue;
        }
        if (n == 1 && strcmp(cmd[0].argv[0], "exit") == 0) break;
        if (n == 1 && strcmp(cmd[0].argv[0], "cd") == 0) {
            const char *path = cmd[0].argc > 1 ? cmd[0].argv[1] : getenv("HOME");
            if (!path || chdir(path) < 0) perror("cd");
            continue;
        }
        if (n == 1 && strcmp(cmd[0].argv[0], "fg") == 0) {
            if (stopped_count == 0) { puts("정지된 작업이 없습니다."); continue; }
            pid_t pgid = stopped_jobs[--stopped_count];
            if (terminal_ok && tcsetpgrp(STDIN_FILENO, pgid) < 0)
                perror("tcsetpgrp(fg)");
            if (kill(-pgid, SIGCONT) < 0) perror("SIGCONT");
            wait_foreground(pgid);
            continue;
        }
        run_commands(cmd, n, background);
    }
    /* 정지된 자식 프로세스 정리 */
    for (int i = 0; i < stopped_count; i++) {
        kill(-stopped_jobs[i], SIGCONT);
        kill(-stopped_jobs[i], SIGTERM);
    }
    free(line);
    return 0;
}

