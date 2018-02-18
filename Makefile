VERSION=0.2

CC = clang
HOSTCC = $(CC)
LOCAL_CFLAGS = -Wall -Werror -Wextra -Wpedantic -g
CFLAGS = -fPIC -O3
SHARED_OBJECTS=src/error.o src/tuntap.o src/memory.o src/bits.o src/base64.o src/exec.o src/websocket.o src/utils.o
OBJECTS=src/main.o $(SHARED_OBJECTS) src/socket.o src/event.o src/io.o src/uwsgi.o src/sha1.o src/macmap.o

OS=$(shell uname)
ifeq ($(OS), Darwin)
	LIBS+=-framework CoreFoundation
	CFLAGS+=-arch i386 -arch x86_64
endif

LIBS+=-ltls $$(config/libs)

all: vpn443 vpn443-client

config/libs: config/libs.c src/config-arc4random.h
	$(HOSTCC) -o $@ $<

src/event.c: src/config-event.h

src/random.c: src/config-arc4random.h

src/%.o: src/%.c src/vpn443.h
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

vpn443: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) -o vpn443 $(OBJECTS) $(SERVER_LIBS)

vpn443-static: $(OBJECTS)
	$(CC) -static $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) -o vpn443 $(OBJECTS) $(SERVER_LIBS)

vpn443-client: src/client.o src/ssl.o src/random.o config/libs $(SHARED_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) -o vpn443-client src/client.o src/random.o src/ssl.o $(SHARED_OBJECTS) $(LIBS)

linux-tarball: vpn443-static
	tar zcvf vpn443-$(VERSION)-linux-$(shell uname -m).tar.gz vpn443

osxpkg: vpn443 vpn443-client
	mkdir -p dist/usr/bin
	cp vpn443 vpn443-client dist/usr/bin
	pkgbuild --root dist --identifier it.unbit.vpn443 vpn443-$(VERSION)-osx.pkg

clean:
	rm -rf src/*.o
	rm -rf vpn443 vpn443-client
	rm -rf config/*.o config/libs
	rm -rf src/config-event.h src/config-arc4random.h

src/config-event.h:
	echo "#undef HAVE_EPOLL" > src/config-event.h
	echo "#undef HAVE_KQUEUE" >> src/config-event.h
	$(CC) $(CFLAGS) -c -o config/epoll.o config/epoll.c >/dev/null 2>&1 && printf "#define HAVE_EPOLL 1\n#define HAVE_KQUEUE 0\n" >> src/config-event.h || exit 0
	$(CC) $(CFLAGS) -c -o config/kqueue.o config/kqueue.c >/dev/null 2>&1 && echo "#define HAVE_KQUEUE 1\n#define HAVE_EPOLL 0" >> src/config-event.h || exit 0
	printf "#if !defined(HAVE_EPOLL) && !defined(HAVE_KQUEUE)\n#error \"Need either epoll or kqueue\"\n#endif\n" >> src/config-event.h

src/config-arc4random.h:
	echo "#undef HAVE_ARC4RANDOM_NATIVE" > src/config-arc4random.h
	echo "#undef HAVE_ARC4RANDOM_LIBBSD" >> src/config-arc4random.h
	$(CC) $(CFLAGS) -c -o config/arc4random_native.o config/arc4random_native.c >/dev/null 2>&1 && printf "#define HAVE_ARC4RANDOM_NATIVE 1\n#define HAVE_ARC4RANDOM_LIBBSD 0\n" >> src/config-arc4random.h || exit 0
	$(CC) $(CFLAGS) -c -o config/arc4random_libbsd.o config/arc4random_libbsd.c >/dev/null 2>&1 && echo "#define HAVE_ARC4RANDOM_LIBBSD 1\n#define HAVE_ARC4RANDOM_NATIVE 0" >> src/config-arc4random.h || exit 0
	printf "#if !defined(HAVE_ARC4RANDOM_NATIVE) && !defined(HAVE_ARC4RANDOM_LIBBSD)\n#error \"Need a source for arc4random\"\n#endif\n" >> src/config-arc4random.h
