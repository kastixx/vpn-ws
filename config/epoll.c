#include <sys/epoll.h>

int main(void)
{
	int ret = epoll_create(1);
	return ret == 0;
}
