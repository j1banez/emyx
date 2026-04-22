#ifndef _VMM_H
#define _VMM_H

#include <stddef.h>

#define VMM_BOOTSTRAP_LIMIT (16u * 1024u * 1024u)

void vmm_init(size_t limit);

#endif
