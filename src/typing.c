#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#define COUNT 3
int main(void) {
    const char *sentences[COUNT] = {"Linux is fun.", "Practice makes perfect.", "Processes communicate with pipes."};
    char input[256]; size_t errors = 0, chars = 0;
    struct timespec start, end;
    puts("각 문장을 입력하고 Enter를 누르세요.");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < COUNT; i++) {
        printf("문장 %d: %s\n입력: ", i+1, sentences[i]); fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) return 1;
        input[strcspn(input, "\n")] = '\0';
        size_t a = strlen(sentences[i]), b = strlen(input);
        size_t max = a > b ? a : b;
        for (size_t j = 0; j < max; j++) if (j >= a || j >= b || sentences[i][j] != input[j]) errors++;
        chars += b;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double seconds = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1e9;
    double cpm = seconds > 0 ? chars * 60.0 / seconds : 0;
    printf("오타(위치별 불일치): %zu개, 시간: %.1f초, 평균 타수: %.1f타/분\n", errors, seconds, cpm);
    return 0;
}
