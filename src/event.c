#include "config-event.h"
#include "vpn443.h"

#if HAVE_EPOLL

#include <sys/epoll.h>

int vpn_ws_event_queue(int n) {
	int ret = epoll_create(n);
	if (ret < 0) {
		vpn_ws_error("vpn_ws_event_queue()/epoll_create()");
		return -1;
	}
	return ret;
}

int vpn_ws_event_read_to_write(int queue, int fd) {
	struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = fd;
        int ret = epoll_ctl(queue, EPOLL_CTL_MOD, fd, &ev);
        if (ret < 0) {
                vpn_ws_error("vpn_ws_event_read_to_write()/epoll_ctl()");
                return -1;
        }
        return ret;
}

int vpn_ws_event_write_to_read(int queue, int fd) {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        int ret = epoll_ctl(queue, EPOLL_CTL_MOD, fd, &ev);
        if (ret < 0) {
                vpn_ws_error("vpn_ws_event_read_to_write()/epoll_ctl()");
                return -1;
        }
        return ret;
}

int vpn_ws_event_add_read(int queue, int fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	int ret = epoll_ctl(queue, EPOLL_CTL_ADD, fd, &ev);
	if (ret < 0) {
		vpn_ws_error("vpn_ws_event_add_read()/epoll_ctl()");	
		return -1;
	}
	return ret;
}

int vpn_ws_event_wait(int queue, void *events) {
	int ret = epoll_wait(queue, events, 64, -1);
	if (ret < 0) {
		vpn_ws_error("vpn_ws_event_wait()/epoll_wait()");
                return -1;
	}
	return ret;
}

void *vpn_ws_event_events(int n) {
	return vpn_ws_malloc(sizeof(struct epoll_event) * n);
}

int vpn_ws_event_fd(void *events, int i) {
	struct epoll_event *epoll_events = (struct epoll_event *) events;
	return epoll_events[i].data.fd;
}

#elif HAVE_KQUEUE

#include <sys/event.h>

int vpn_ws_event_queue(int n) {
        int ret = kqueue();
        if (ret < 0) {
                vpn_ws_error("vpn_ws_event_queue()/kqueue()");
                return -1;
        }
        return ret;
}

int vpn_ws_event_read_to_write(int queue, int fd) {
	struct kevent kev;

	EV_SET(&kev, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
        if (kevent(queue, &kev, 1, NULL, 0, NULL) < 0) {
		vpn_ws_error("vpn_ws_event_read_to_write()/kevent()");
                return -1;
        }

        EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
        if (kevent(queue, &kev, 1, NULL, 0, NULL) < 0) {
		vpn_ws_error("vpn_ws_event_read_to_write()/kevent()");
                return -1;
        }
        return 0;
}

int vpn_ws_event_write_to_read(int queue, int fd) {
	struct kevent kev;
        
        EV_SET(&kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
        if (kevent(queue, &kev, 1, NULL, 0, NULL) < 0) {
                vpn_ws_error("vpn_ws_event_write_to_read()/kevent()");
                return -1;
        }
        
        EV_SET(&kev, fd, EVFILT_READ, EV_ADD, 0, 0, 0);
        if (kevent(queue, &kev, 1, NULL, 0, NULL) < 0) {
                vpn_ws_error("vpn_ws_event_write_to_read()/kevent()");
                return -1;
        }
        return 0;
}

int vpn_ws_event_add_read(int queue, int fd) {
	struct kevent kev;

	EV_SET(&kev, fd, EVFILT_READ, EV_ADD, 0, 0, 0);
        if (kevent(queue, &kev, 1, NULL, 0, NULL) < 0) {
                vpn_ws_error("vpn_ws_event_add_read()/kevent()");
                return -1;
        }
        return 0;
}

int vpn_ws_event_wait(int queue, void *events) {
        int ret = kevent(queue, NULL, 0, events, 64, NULL);
        if (ret < 0) {
                vpn_ws_error("vpn_ws_event_wait()/kevent()");
        }
        return ret;
}

void *vpn_ws_event_events(int n) {
        return vpn_ws_malloc(sizeof(struct kevent) * n);
}

int vpn_ws_event_fd(void *events, int i) {
        struct kevent *k_events = (struct kevent *) events;
        return k_events[i].ident;
}
#else
#error "Not supported"
#endif
