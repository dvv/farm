#
# build haproxy and stunnel
#
# dronnikov@gmail.com 2011
#

HAPROXY=haproxy-1.5-dev6
LIBEV=libev-4.04
STUD=stud-latest
REDIS=redis-latest
ARCH=$(shell uname)
ifneq ($(ARCH),Darwin)
	MONGO_OS=osx
	MONGO=mongodb-osx-i386-1.8.3
else
	MONGO_OS=linux
	MONGO=mongodb-linux-i686-1.8.3
endif

ROOT=$(shell pwd)

all: check bin

check:
	# FIXME: non-debians have no dpkg.
	dpkg -s runit >/dev/null
	dpkg -s ipsvd >/dev/null

#bin: $(MONGO)/bin/mongo $(HAPROXY)/haproxy $(STUD)/stud $(REDIS)/src/redis-server
bin: $(HAPROXY)/haproxy $(STUD)/stud $(REDIS)/src/redis-server

$(HAPROXY)/haproxy: $(HAPROXY)
	make -C $^ TARGET=generic

$(HAPROXY): $(HAPROXY).tar.gz
	tar xzpf $^

$(HAPROXY).tar.gz:
	wget http://haproxy.1wt.eu/download/1.5/src/devel/$(HAPROXY).tar.gz

$(STUD)/stud: $(STUD) $(LIBEV)/.libs/libev.a
	#CFLAGS='-I$(ROOT)/$(LIBEV) -L$(ROOT)/$(LIBEV)/.libs' make -C $(STUD)
	# N.B. to statically link with libev.a, had to override stud's Makefile
	( cd $(STUD) ; gcc -O2 -g -std=c99 -fno-strict-aliasing -Wall -W -I$(ROOT)/$(LIBEV) -I. -o stud ringbuffer.c stud.c -D_GNU_SOURCE $(ROOT)/$(LIBEV)/.libs/libev.a -lm -lssl -lcrypto )

$(STUD): $(STUD).tar.gz
	tar xzpf $^
	mv bumptech* $@

$(STUD).tar.gz:
	wget https://github.com/bumptech/stud/tarball/master -O $@

$(LIBEV)/.libs/libev.a: $(LIBEV)
	(cd $^ ; ./configure)
	make -C $^

$(LIBEV): $(LIBEV).tar.gz
	tar xzpf $^

$(LIBEV).tar.gz:
	wget http://dist.schmorp.de/libev/$(LIBEV).tar.gz

$(REDIS)/src/redis-server: $(REDIS)
	make -C $^

$(REDIS): $(REDIS).tar.gz
	tar xzpf $^
	mv antirez* $@

$(REDIS).tar.gz:
	wget https://github.com/antirez/redis/tarball/master -O $@

$(MONGO)/bin/mongo: $(MONGO)
	touch -c $@

$(MONGO): $(MONGO).tgz
	tar xzpf $^

$(MONGO).tgz:
	wget http://fastdl.mongodb.org/$(MONGO_OS)/$(MONGO).tgz -O $@

#$(BUSYBOX):
#	wget http://landley.net/aboriginal/downloads/binaries/root-filesystems/simple-root-filesystem-i686.tar.bz2

install: bin
	install -s $(HAPROXY)/haproxy $(STUD)/stud $(REDIS)/src/redis-server $(REDIS)/src/redis-cli $(MONGO)/bin/* /usr/local/bin
	-useradd haproxy
	-useradd stud
	-useradd redis
	-useradd mongo
	cp -a runit/* /etc/service

uninstall:
	rm -fr /usr/local/bin/haproxy /usr/local/bin/stud /usr/local/bin/redis-* /usr/local/bin/mongo* /usr/local/bin/bsondump /etc/service/haproxy /etc/service/stud /etc/service/redis /etc/service/mongo
	-userdel mongo
	-userdel redis
	-userdel stud
	-userdel haproxy

clean:
	rm -fr $(HAPROXY) $(STUD) $(LIBEV) $(REDIS) $(MONGO)
	rm -fr $(HAPROXY).tar.gz $(STUD).tar.gz $(LIBEV).tar.gz $(REDIS).tar.gz $(MONGO).tgz

.PHONY: all check bin lib install uninstall clean
