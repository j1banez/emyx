#include <string.h>
#include <stdint.h>

#include <kernel/printk.h>
#include <kernel/shell.h>
#include <kernel/timer.h>
#include <kernel/tty.h>

typedef struct {
    const char *name;
    const char *desc;
    void (*fn)(void);
} shell_cmd;

static void shell_exec(void);
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_ticks(void);

static char buffer[128];
static uint32_t length;

static const shell_cmd commands[] = {
    { "help", "List commands", cmd_help },
    { "clear", "Clear screen", cmd_clear },
    { "ticks", "Show timer", cmd_ticks },
    { "irq", "Show IRQ info", cmd_help },
};

void shell_init(void)
{
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    printk("emyx>");
}

void shell_on_char(char c)
{
    switch (c) {
        case '\n':
            printk("%c", c);
            shell_exec();
            shell_init();
            break;
        case '\b':
            if (length > 0) {
                length--;
                buffer[length] = '\0';
                terminal_backspace();
            }
            break;
        default:
            if (length < sizeof(buffer) - 1) {
                buffer[length++] = c;
                buffer[length] = '\0';
                printk("%c", c);
            }
            break;
    }
}

static void shell_exec(void)
{
    // TODO: trim buffer

    if (strlen(buffer) == 0)
        return;

    size_t n = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < n; i++) {
        shell_cmd cmd = commands[i];
        size_t cmd_len = strlen(cmd.name);

        if (cmd_len == length && memcmp(buffer, cmd.name, cmd_len) == 0) {
            cmd.fn();
            return;
        }
    }

    printk("Unknown command\n");
}

static void cmd_help(void)
{
    printk("Available commands:\n");

    size_t n = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < n; i++) {
        printk("- %s: %s\n", commands[i].name, commands[i].desc);
    }
}

static void cmd_clear(void)
{
    terminal_init();
}

static void cmd_ticks(void)
{
    uint32_t ticks = timer_get_ticks();

    // Assumes timer is configured to 100Hz
    printk("Ticks: %u (%x)\nUptime: %us\n", ticks, ticks, ticks / 100);
}
