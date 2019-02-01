
#ifdef DEBUG

#include <stdio.h>
#include <assert.h>

void __throw_out_of_range(const char *msg)
{
    printf("!!! %s !!!\n", msg);
    assert(0);
}

#endif
