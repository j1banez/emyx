#include "user.h"

int main(void)
{
    write(SYS_FD_STDOUT, "hello from C\n", 13);
    return 0;
}
