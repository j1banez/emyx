#include "user.h"

int main(void)
{
    int pid;
    int status;

    pid = spawn("/bin/shell");
    if (pid < 0)
        return 1;

    status = wait(pid);
    if (status < 0)
        return 1;

    return status;
}
