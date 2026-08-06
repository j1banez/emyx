#include <stdint.h>

#include "user.h"

#define INPUT_SIZE 32u

int main(void)
{
    char input[INPUT_SIZE];
    uint32_t length;
    int c;
    int pid;

    length = 0;
    write(SYS_FD_STDOUT, "emyx$ ", 6);

    while (1) {
        c = getchar();
        if (c == 0) {
            yield();
            continue;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                write(SYS_FD_STDOUT, "\b", 1);
            }
            continue;
        }

        if (c == '\n') {
            write(SYS_FD_STDOUT, "\n", 1);
            input[length] = '\0';

            // `quit` builtin
            if (length == 4 && input[0] == 'q' && input[1] == 'u' &&
                    input[2] == 'i' && input[3] == 't')
                return 0;

            if (length > 0) {
                pid = spawn(input);
                if (pid < 0) {
                    write(SYS_FD_STDOUT,
                        "shell: failed to run program\n", 29);
                } else if (wait(pid) < 0) {
                    write(SYS_FD_STDOUT,
                        "shell: failed to wait for program\n", 34);
                }
            }

            length = 0;
            write(SYS_FD_STDOUT, "emyx$ ", 6);
            continue;
        }

        if (c < ' ' || c > '~' || length == sizeof(input) - 1)
            continue;

        input[length] = (char)c;
        write(SYS_FD_STDOUT, &input[length], 1);
        length++;
    }
}
