#include "libc.h"

/* write <file> <text...>   -- appends "<text>\n" to the file, creating it. */
int main(int argc, char **argv)
{
    if (argc < 2) {
        puts("usage: write <file> <text...>\n");
        return 1;
    }

    int fd = open(argv[1], O_WRCREAT);
    if (fd < 0) {
        puts("write: cannot open\n");
        return 1;
    }

    struct statbuf st;
    stat(argv[1], &st);
    lseek(fd, st.size);

    for (int i = 2; i < argc; i++) {
        if (i > 2)
            write(fd, " ", 1);
        write(fd, argv[i], strlen(argv[i]));
    }
    write(fd, "\n", 1);
    close(fd);
    return 0;
}
