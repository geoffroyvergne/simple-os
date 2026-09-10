#include "shell.h"
#include "keyboard.h"
#include "console.h"
#include "kprintf.h"
#include "string.h"
#include "io.h"
#include "pit.h"

#define LINE_MAX 256

static void cmd_help(void)
{
    kprintf("commands:\n");
    kprintf("  help          this text\n");
    kprintf("  echo <text>   print text\n");
    kprintf("  clear         clear the screen\n");
    kprintf("  ticks         PIT tick count since boot\n");
    kprintf("  uptime        seconds since boot\n");
    kprintf("  reboot        reset via the 8042 controller\n");
}

static void execute(char *line)
{
    while (*line == ' ')
        line++;

    if (*line == '\0')
        return;

    char *arg = line;
    while (*arg && *arg != ' ')
        arg++;
    int has_arg = (*arg == ' ');
    if (has_arg)
        *arg++ = '\0';
    while (*arg == ' ')
        arg++;

    if (strcmp(line, "help") == 0) {
        cmd_help();
    } else if (strcmp(line, "echo") == 0) {
        kprintf("%s\n", has_arg ? arg : "");
    } else if (strcmp(line, "clear") == 0) {
        console_clear();
    } else if (strcmp(line, "ticks") == 0) {
        kprintf("%u\n", (uint32_t)pit_ticks());
    } else if (strcmp(line, "uptime") == 0) {
        uint32_t t = (uint32_t)pit_ticks();
        kprintf("%u.%02u s\n", t / 100, t % 100);
    } else if (strcmp(line, "reboot") == 0) {
        kprintf("rebooting...\n");
        pit_sleep_ms(200);
        outb(0x64, 0xFE);
    } else {
        kprintf("unknown command: %s\n", line);
    }
}

void shell_run(void)
{
    char line[LINE_MAX];

    kprintf("\ninteractive shell. type 'help'.\n\n");

    for (;;) {
        console_set_color(VGA_LCYAN, VGA_BLACK);
        kprintf("simple-os> ");
        console_set_color(VGA_LGRAY, VGA_BLACK);

        int n = keyboard_readline(line, sizeof(line));
        if (n < 0) {
            kprintf("^C\n");
            continue;
        }
        execute(line);
    }
}
