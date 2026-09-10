#include "libc.h"

#define MAXARGS 16

static int tokenize(char *line, char **argv)
{
    int argc = 0;
    char *p = line;
    while (*p && argc < MAXARGS - 1) {
        while (*p == ' ')
            *p++ = '\0';
        if (!*p)
            break;
        argv[argc++] = p;
        while (*p && *p != ' ')
            p++;
    }
    argv[argc] = 0;
    return argc;
}

static void help(void)
{
    puts("built-in: help, exit, reboot\n");
    puts("anything else is run from the filesystem, e.g.:\n");
    puts("  ls   cat <f>   hexdump <f>   echo ...   free\n");
    puts("  write <f> <text>   rm <f>   hello ...\n");
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char line[256];

    puts("\nSimpleOS shell (pid ");
    putint(getpid());
    puts("). type 'help'.\n");

    for (;;) {
        puts("$ ");
        int n = read(0, line, sizeof(line) - 1);
        if (n <= 0)
            continue;
        line[n] = '\0';

        char *av[MAXARGS];
        int ac = tokenize(line, av);
        if (ac == 0)
            continue;

        if (!strcmp(av[0], "help")) {
            help();
        } else if (!strcmp(av[0], "exit")) {
            return 0;
        } else if (!strcmp(av[0], "reboot")) {
            reboot();
        } else {
            int rc = spawn(av[0], av);
            if (rc == -2)
                puts("no such command\n");
            else if (rc < 0)
                puts("spawn failed\n");
            else if (rc != 0) {
                puts("[exit ");
                putint(rc);
                puts("]\n");
            }
        }
    }
}
