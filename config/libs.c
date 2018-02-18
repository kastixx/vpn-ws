#include <stdio.h>
#include "../src/config-arc4random.h"

int
main(void)
{
#if HAVE_ARC4RANDOM_LIBBSD
	fputs(" -lbsd", stdout);
#endif
	fflush(stdout);
}
