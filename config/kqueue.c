#include <sys/event.h>

int main(void)
{
	int ret = kqueue();
	return ret == 0;
}
