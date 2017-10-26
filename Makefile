VERSION=0.2

CC = clang
LOCAL_CFLAGS = -Wall -Werror -Wextra -Wpedantic -g
CFLAGS = -fPIC -O3
SHARED_OBJECTS=src/error.o src/tuntap.o src/memory.o src/bits.o src/base64.o src/exec.o src/websocket.o src/utils.o
OBJECTS=src/main.o $(SHARED_OBJECTS) src/socket.o src/event.o src/io.o src/uwsgi.o src/sha1.o src/macmap.o

ifeq ($(OS), Windows_NT)
	LIBS+=-lws2_32 -lsecur32
	SERVER_LIBS = -lws2_32
else
	OS=$(shell uname)
	ifeq ($(OS), Darwin)
		LIBS+=-framework Security -framework CoreFoundation
		CFLAGS+=-arch i386 -arch x86_64
	else
		LIBS+=-lcrypto -lssl
	endif
endif

all: vpn-ws vpn-ws-client

src/event.c: src/config-event.h


src/%.o: src/%.c src/vpn-ws.h
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

vpn-ws: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) -o vpn-ws $(OBJECTS) $(SERVER_LIBS)

vpn-ws-static: $(OBJECTS)
	$(CC) -static $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) -o vpn-ws $(OBJECTS) $(SERVER_LIBS)

vpn-ws-client: src/client.o src/ssl.o $(SHARED_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) -o vpn-ws-client src/client.o src/ssl.o $(SHARED_OBJECTS) $(LIBS)

linux-tarball: vpn-ws-static
	tar zcvf vpn-ws-$(VERSION)-linux-$(shell uname -m).tar.gz vpn-ws

osxpkg: vpn-ws vpn-ws-client
	mkdir -p dist/usr/bin
	cp vpn-ws vpn-ws-client dist/usr/bin
	pkgbuild --root dist --identifier it.unbit.vpn-ws vpn-ws-$(VERSION)-osx.pkg

clean:
	rm -rf src/*.o vpn-ws vpn-ws-client configure/*.o src/config-event.h

src/config-event.h:
	echo "#undef HAVE_EPOLL" > src/config-event.h
	echo "#undef HAVE_KQUEUE" >> src/config-event.h
	$(CC) $(CFLAGS) -c -o config/epoll.o config/epoll.c >/dev/null 2>&1 && echo "#define HAVE_EPOLL 1" >> src/config-event.h || exit 0
	$(CC) $(CFLAGS) -c -o config/kqueue.o config/kqueue.c >/dev/null 2>&1 && echo "#define HAVE_KQUEUE 1" >> src/config-event.h || exit 0
