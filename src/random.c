#include "config-arc4random.h"

#include <stdint.h>

#ifdef HAVE_ARC4RANDOM_LIBBSD
#include <bsd/stdlib.h>
#elif defined(HAVE_ARC4RANDOM_NATIVE)
#include <stdlib.h>
#endif

#include "vpn443-random.h"

uint32_t
vpn443_rand()
{
	return arc4random();
}
