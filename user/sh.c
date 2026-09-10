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
    puts("anything else runs from the filesystem: ls, cat, hexdump, echo,\n");
    puts("write, rm, free, count, hello\n");
    puts("append ' &' to run a command in the background\n");
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
        /* reap any finished background jobs without blocking (pid 0 = poll) */
        int code, bg;
        while ((bg = wait(0, &code)) > 0) {
            puts("[bg pid ");
            putint(bg);
            puts(" exit ");
            putint(code);
            puts("]\n");
        }

        puts("$ ");
        int n = read(0, line, sizeof(line) - 1);
        if (n <= 0)
            continue;
        line[n] = '\0';

        char *av[MAXARGS];
        int ac = tokenize(line, av);
        if (ac == 0)
            continue;

        int background = 0;
        if (ac > 1 && !strcmp(av[ac - 1], "&")) {
            av[--ac] = 0;
            background = 1;
        }

        if (!strcmp(av[0], "help")) {
            help();
            continue;
        }
        if (!strcmp(av[0], "exit"))
            return 0;
        if (!strcmp(av[0], "reboot"))
            reboot();

        int pid = spawn(av[0], av);
        if (pid == -2) {
            puts("no such command\n");
        } else if (pid < 0) {
            puts("spawn failed\n");
        } else if (background) {
            puts("[pid ");
            putint(pid);
            puts("]\n");
        } else {
            int code;
            wait(pid, &code);
            if (code != 0) {
                puts("[exit ");
                putint(code);
                puts("]\n");
            }
        }
    }
}
