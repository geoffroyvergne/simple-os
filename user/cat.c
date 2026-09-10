#include "libc.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        puts("usage: cat <file> ...\n");
        return 1;
    }

    int status = 0;
    char buf[256];

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            puts("cat: ");
            puts(argv[i]);
            puts(": not found\n");
            status = 1;
            continue;
        }
        int n;
        while ((n = read(fd, buf, sizeof(buf))) > 0)
            write(1, buf, (size_t)n);
        close(fd);
    }
    return status;
}
