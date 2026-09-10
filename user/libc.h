#pragma once

typedef unsigned int size_t;

#define O_RDONLY  0
#define O_WRCREAT 1

struct dirent {
    char     name[40];
    unsigned size;
};

struct statbuf {
    unsigned size;
    unsigned capacity;
};

struct sysinfo {
    unsigned ram_kb;
    unsigned ram_free_kb;
    unsigned heap_total_kb;
    unsigned heap_used_kb;
    unsigned uptime_ticks;
};

/* syscalls */
void exit(int code) __attribute__((noreturn));
int  getpid(void);
int  write(int fd, const void *buf, size_t n);
int  read(int fd, void *buf, size_t n);
int  open(const char *path, int flags);
int  close(int fd);
int  lseek(int fd, unsigned offset);
int  readdir(int index, struct dirent *out);
int  stat(const char *path, struct statbuf *out);
int  spawn(const char *path, char **argv);     /* returns a pid, or < 0 */
int  wait(int pid, int *code);                  /* pid <= 0 = any child; blocks */
void sleep(unsigned ms);
int  unlink(const char *path);
int  sysinfo(struct sysinfo *out);
void reboot(void) __attribute__((noreturn));

/* helpers */
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
void  *memcpy(void *d, const void *s, size_t n);
void   putchar(char c);
void   puts(const char *s);        /* no newline added */
void   putint(int v);
