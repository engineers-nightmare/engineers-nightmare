#include <stdio.h>
#include <assert.h>
#include "../src/space.h"

int
main(void)
{
    block *b;
    sub_space<4> space;

    b = space.get_block(0, 0, 0);

    assert(b == 0);

}
