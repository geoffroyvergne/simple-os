#include "libc.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        puts("usage: rm <file> ...\n");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            puts("rm: ");
            puts(argv[i]);
            puts(": not found\n");
            status = 1;
        }
    }
    return status;
}
