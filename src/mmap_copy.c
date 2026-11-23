#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr,"사용법: %s 원본 복사본\n", argv[0]); return 1; }
    int src = open(argv[1], O_RDONLY); if (src < 0) { perror("open src"); return 1; }
    struct stat st; if (fstat(src,&st)<0) { perror("fstat"); close(src); return 1; }
    int dst = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) { perror("open dst"); close(src); return 1; }
    if (ftruncate(dst, st.st_size)<0) { perror("ftruncate"); goto fail; }
    if (st.st_size == 0) { puts("빈 파일 복사 완료"); close(src); close(dst); return 0; }
    void *from = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, src, 0);
    if (from == MAP_FAILED) { perror("mmap src"); goto fail; }
    void *to = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, dst, 0);
    if (to == MAP_FAILED) { perror("mmap dst"); munmap(from, st.st_size); goto fail; }
    memcpy(to, from, st.st_size);
    if (msync(to, st.st_size, MS_SYNC)<0) perror("msync");
    munmap(from, st.st_size); munmap(to, st.st_size);
    printf("%lld바이트 복사 완료\n",(long long)st.st_size);
    close(src); close(dst); return 0;
fail: close(src); close(dst); return 1;
}
