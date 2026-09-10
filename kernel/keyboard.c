#include "keyboard.h"
#include "io.h"
#include "interrupts.h"
#include "pic.h"
#include "kprintf.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

/* Scancode set 1, US QWERTY. Index by make code (0x00..0x7F). 0 = no char. */
static const char MAP_BASE[128] = {
    [0x01] = 27,
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';',
    [0x28] = '\'', [0x29] = '`', [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x37] = '*', [0x39] = ' ',
    [0x4A] = '-', [0x4E] = '+',
};

static const char MAP_SHIFT[128] = {
    [0x01] = 27,
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L', [0x27] = ':',
    [0x28] = '"', [0x29] = '~', [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B',
    [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x37] = '*', [0x39] = ' ',
    [0x4A] = '-', [0x4E] = '+',
};

/* Single-producer (IRQ1) / single-consumer (kernel thread) ring buffer. */
#define RB_SIZE 256
static volatile uint8_t rb[RB_SIZE];
static volatile uint32_t rb_head;   /* written only by the IRQ */
static volatile uint32_t rb_tail;   /* written only by the consumer */

static int shift, ctrl, caps, e0_prefix;

static void rb_push(uint8_t c)
{
    uint32_t next = (rb_head + 1) % RB_SIZE;
    if (next != rb_tail) {
        rb[rb_head] = c;
        rb_head = next;
    }
}

static void process_scancode(uint8_t sc)
{
    if (sc == 0xE0) {
        e0_prefix = 1;
        return;
    }

    int released = sc & 0x80;
    uint8_t code = sc & 0x7F;

    if (e0_prefix) {
        /* Extended keys (arrows, keypad Enter, right ctrl, ...). Only right
         * ctrl is tracked for now; the rest are consumed. */
        e0_prefix = 0;
        if (code == 0x1D)
            ctrl = !released;
        return;
    }

    switch (code) {
    case 0x2A: case 0x36:                 /* Left / Right Shift */
        shift = !released;
        return;
    case 0x1D:                            /* Left Ctrl */
        ctrl = !released;
        return;
    case 0x3A:                            /* Caps Lock (toggle on press) */
        if (!released)
            caps = !caps;
        return;
    }

    if (released)
        return;

    char c = shift ? MAP_SHIFT[code] : MAP_BASE[code];
    if (!c)
        return;

    if (caps && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
        c ^= 0x20;

    if (ctrl && ((c | 0x20) >= 'a' && (c | 0x20) <= 'z'))
        c = (char)(c & 0x1F);            /* Ctrl-A..Z -> 0x01..0x1A */

    rb_push((uint8_t)c);
}

static void on_key(struct registers *r)
{
    (void)r;
    /* Drain every pending byte: a fast key burst (e.g. a Shift+key chord is
     * four scancodes) can queue several bytes for a single IRQ line assert. */
    uint8_t st;
    while ((st = inb(KBD_STATUS)) & 1) {
        uint8_t data = inb(KBD_DATA);
        if (!(st & 0x20))               /* skip bytes from the aux (mouse) port */
            process_scancode(data);
    }
}

void keyboard_init(void)
{
    /* Drain any bytes the controller latched during boot. */
    while (inb(KBD_STATUS) & 1)
        (void)inb(KBD_DATA);

    irq_install_handler(1, on_key);
    pic_clear_mask(1);
}

int keyboard_trygetchar(void)
{
    if (rb_tail == rb_head)
        return -1;
    uint8_t c = rb[rb_tail];
    rb_tail = (rb_tail + 1) % RB_SIZE;
    return c;
}

char keyboard_getchar(void)
{
    for (;;) {
        __asm__ volatile("cli");
        if (rb_tail != rb_head) {
            uint8_t c = rb[rb_tail];
            rb_tail = (rb_tail + 1) % RB_SIZE;
            __asm__ volatile("sti");
            return (char)c;
        }
        /* sti; hlt is atomic: no interrupt can slip in between, so a key that
         * arrives right after the check still wakes us. */
        __asm__ volatile("sti; hlt");
    }
}

int keyboard_readline(char *buf, size_t size)
{
    size_t len = 0;

    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            kputchar('\n');
            buf[len] = '\0';
            return (int)len;
        }
        if (c == 0x03) {                 /* Ctrl-C */
            buf[0] = '\0';
            return -1;
        }
        if (c == '\b' || c == 0x7F) {    /* Backspace */
            if (len > 0) {
                len--;
                kputs("\b \b");
            }
            continue;
        }
        if (c == 0x15) {                 /* Ctrl-U: kill line */
            while (len > 0) {
                len--;
                kputs("\b \b");
            }
            continue;
        }
        if (c < 0x20)                    /* ignore other control chars */
            continue;
        if (len + 1 < size) {
            buf[len++] = c;
            kputchar(c);
        }
    }
}
