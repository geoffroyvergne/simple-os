#pragma once

typedef unsigned int size_t;

void exit(int code) __attribute__((noreturn));
int  getpid(void);
int  write(int fd, const void *buf, size_t n);
int  read(int fd, void *buf, size_t n);

size_t strlen(const char *s);
void   puts(const char *s);      /* no newline added */
void   putint(int v);
void   putchar(char c);
int    readline(char *buf, size_t n);   /* reads a line from fd 0 */
