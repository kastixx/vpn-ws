#include "vpn-ws.h"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <net/if_dl.h>
#include <sys/sysctl.h>
#endif

#if defined(__linux__)

#include <linux/if_tun.h>
#define TUNTAP_DEVICE "/dev/net/tun"

int vpn_ws_tuntap(char *name) {
	struct ifreq ifr;
        int fd = open(TUNTAP_DEVICE, O_RDWR);
	if (fd < 0) {
		vpn_ws_error("vpn_ws_tuntap()/open()");
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));

        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);

	if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
		vpn_ws_error("vpn_ws_tuntap()/ioctl()");
                return -1;
        }

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		vpn_ws_error("vpn_ws_tuntap()/ioctl()");
                return -1;
	}

	// copy MAC address
	memcpy(vpn_ws_conf.tuntap_mac, ifr.ifr_hwaddr.sa_data, 6);
	//printf("%x %x\n", vpn_ws_conf.tuntap_mac[0], vpn_ws_conf.tuntap_mac[1]);

	return fd;
}

#else

#if defined(__APPLE__)
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
// like linux
#define SIOCGIFHWADDR   0x8927
#endif

int vpn_ws_tuntap(char *name) {
	int fd = -1;
#if defined(__APPLE__)
	// is it it.unbit.utap ?
	if (vpn_ws_is_a_number(name)) {
		fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
		if (fd < 0) {
			vpn_ws_error("vpn_ws_tuntap()/socket()");
			return -1;
		}
		struct ctl_info info;
        	memset(&info, 0, sizeof(info));
        	strncpy(info.ctl_name, "it.unbit.utap", sizeof(info.ctl_name));
        	if (ioctl(fd, CTLIOCGINFO, &info)) {
			vpn_ws_error("vpn_ws_tuntap()/ioctl()");
			close(fd);
			return -1;
		}
		struct sockaddr_ctl       addr;
        	memset(&addr, 0, sizeof(addr));
        	addr.sc_len = sizeof(addr);
        	addr.sc_family = AF_SYSTEM;
        	addr.ss_sysaddr = AF_SYS_CONTROL;
        	addr.sc_id = info.ctl_id;
        	addr.sc_unit = atoi(name);

		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
			vpn_ws_error("vpn_ws_tuntap()/connect()");
			close(fd);
			return -1;
		}

		socklen_t mac_len = 6;
		if (getsockopt(fd, SYSPROTO_CONTROL, SIOCGIFHWADDR, vpn_ws_conf.tuntap_mac, &mac_len)) {
			vpn_ws_error("vpn_ws_tuntap()/getsockopt()");
                        close(fd);
                        return -1;
		}

		return fd;
	}
	else {
#endif
	fd = open(name, O_RDWR);
	if (fd < 0) {
		vpn_ws_error("vpn_ws_tuntap()/open()");
		return -1;
	}
#if defined(__APPLE__)
	}
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	int mib[6];
	mib[0] = CTL_NET;
	mib[1] = AF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_LINK;
	mib[4] = NET_RT_IFLIST;
	mib[5] = if_nametoindex(name+5);
	if (!mib[5]) {
		vpn_ws_error("vpn_ws_tuntap()/if_nametoindex()");
		close(fd);
		return -1;
	}

	size_t len;
	if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
		vpn_ws_error("vpn_ws_tuntap()/sysctl()");
                close(fd);
                return -1;
	}

	char *buf = vpn_ws_malloc(len);
	if (!buf) {
		close(fd);
		return -1;
	}

	if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
		vpn_ws_error("vpn_ws_tuntap()/sysctl()");
                close(fd);
                return -1;
	}

	struct if_msghdr *ifm = (struct if_msghdr *)buf;
	struct sockaddr_dl *sdl = (struct sockaddr_dl *)(ifm + 1);
	uint8_t *ptr = (uint8_t *)LLADDR(sdl);
        // copy MAC address
        memcpy(vpn_ws_conf.tuntap_mac, ptr, 6);
#endif

	return fd;
}

#endif
