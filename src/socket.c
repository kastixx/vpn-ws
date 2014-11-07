#include "vpn-ws.h"

int vpn_ws_socket_nb(int fd) {
        int arg = fcntl(fd, F_GETFL, NULL);
        if (arg < 0) {
                vpn_ws_error("vpn_ws_socket_nb()/fcntl()");
                return -1;
        }
        arg |= O_NONBLOCK;
        if (fcntl(fd, F_SETFL, arg) < 0) {
                vpn_ws_error("vpn_ws_socket_nb()/fcntl()");
                return -1;
        }
	return 0;
}

int vpn_ws_bind_ipv6(char *name) {
	return -1;
}

int vpn_ws_bind_ipv4(char *name) {
	return -1;
}

int vpn_ws_bind_unix(char *name) {

	// ignore unlink error
	unlink(name);

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		vpn_ws_error("vpn_ws_bind_unix()/socket()");
		return -1;
	}

	struct sockaddr_un s_un;
	memset(&s_un, 0, sizeof(struct sockaddr_un));
	s_un.sun_family = AF_UNIX;
	strncpy(s_un.sun_path, name, sizeof(s_un.sun_path));

	if (bind(fd, (struct sockaddr *) &s_un, sizeof(struct sockaddr_un)) < 0) {
		vpn_ws_error("vpn_ws_bind_unix()/bind()");
		close(fd);
		return -1;
	}

	if (listen(fd, 100) < 0) {
		vpn_ws_error("vpn_ws_bind_unix()/listen()");
                close(fd);
                return -1;
	}

	if (chmod(name, 0666)) {
		vpn_ws_error("vpn_ws_bind_unix()/chmod()");
                close(fd);
                return -1;
	}

	return fd;
}

/*
	this needs to manage AF_UNIX, AF_INET and AF_INET6
*/
int vpn_ws_bind(char *name) {
	char *colon = strchr(name, ':');
	if (!colon) return vpn_ws_bind_unix(name);
	if (name[0] == '[') return vpn_ws_bind_ipv6(name);
	return vpn_ws_bind_ipv4(name);
}

void vpn_ws_peer_accept(int queue, int fd) {
	struct sockaddr_un s_un;
        memset(&s_un, 0, sizeof(struct sockaddr_un));

	socklen_t s_len = sizeof(struct sockaddr_un);

	int client_fd = accept(fd, (struct sockaddr *) &s_un, &s_len);
	if (client_fd < 0) {
		vpn_ws_error("vpn_ws_peer_accept()/accept()");
		return;
	}

	if (vpn_ws_socket_nb(client_fd)) {
		close(client_fd);
		return;
	}

	if (vpn_ws_event_add_read(queue, client_fd)) {
		close(client_fd);
		return;
	}

	// create a new peer structure
	// we use >= so we can lazily allocate memory even if fd is 0
	if (client_fd >= vpn_ws_conf.peers_n) {
		void *tmp = realloc(vpn_ws_conf.peers, sizeof(vpn_ws_peer *) * (client_fd+1));
		if (!tmp) {
			vpn_ws_error("vpn_ws_peer_accept()/realloc()");
			close(client_fd);
                	return;
		}
		uint64_t delta = (client_fd+1) - vpn_ws_conf.peers_n;
		memset(tmp + (sizeof(vpn_ws_peer *) * vpn_ws_conf.peers_n), 0, sizeof(vpn_ws_peer *) * delta);
		vpn_ws_conf.peers_n = client_fd+1;
		vpn_ws_conf.peers = (vpn_ws_peer **) tmp;
	}

	vpn_ws_peer *peer = vpn_ws_calloc(sizeof(vpn_ws_peer));
	if (!peer) {
		close(client_fd);
                return;
	}

	peer->fd = client_fd;

	vpn_ws_conf.peers[client_fd] = peer;

	fprintf(stderr, "added fd %d\n", peer->fd);
}