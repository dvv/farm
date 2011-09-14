#
# build haproxy and stunnel
#
# dronnikov@gmail.com 2011
#

HAPROXY=haproxy-1.5-dev7
STUD=stud-latest
REDIS=redis-latest
WEBFS=webfs-1.21
ARCH=$(shell uname)
ifeq ($(ARCH),Darwin)
	MONGO_OS=osx
	MONGO=mongodb-osx-i386-2.0.0
	STUD_TARGET=stud
else
	MONGO_OS=linux
	MONGO=mongodb-linux-i686-2.0.0
	STUD_TARGET=stud-shared
endif

ROOT=$(shell pwd)

all: check bin

check:
ifneq ($(ARCH),Darwin)
	# FIXME: non-debians have no dpkg.
	dpkg -s runit >/dev/null
	dpkg -s ipsvd >/dev/null
endif

bin: $(MONGO)/bin/mongo $(HAPROXY)/haproxy $(STUD)/$(STUD_TARGET) $(REDIS)/src/redis-server $(WEBFS)/webfsd

$(HAPROXY)/haproxy: $(HAPROXY)
	make -C $^ TARGET=generic

$(HAPROXY):
	wget http://haproxy.1wt.eu/download/1.5/src/devel/$(HAPROXY).tar.gz -O - | tar -xzpf -

$(STUD)/$(STUD_TARGET): $(STUD)
	make -C $^ $(STUD_TARGET)

$(STUD):
	wget https://github.com/dvv/stud/tarball/master -O - | tar -xzpf -
	mv dvv* $@

$(REDIS)/src/redis-server: $(REDIS)
	make -C $^

$(REDIS):
	wget https://github.com/antirez/redis/tarball/master -O - | tar -xzpf -
	mv antirez* $@

$(MONGO)/bin/mongo: $(MONGO)
	touch -c $@

$(MONGO):
	wget http://fastdl.mongodb.org/$(MONGO_OS)/$(MONGO).tgz -O - | tar -xzpf -

$(WEBFS)/webfsd: $(WEBFS)
	make -C $^

$(WEBFS):
	wget http://www.kraxel.org/releases/webfs/$(WEBFS).tar.gz -O - | tar -xzpf -

#$(BUSYBOX):
#	wget http://landley.net/aboriginal/downloads/binaries/root-filesystems/simple-root-filesystem-i686.tar.bz2

install: bin
	install -s $(HAPROXY)/haproxy $(REDIS)/src/redis-server $(REDIS)/src/redis-cli $(WEBFS)/webfsd /usr/local/bin
	install $(MONGO)/bin/* /usr/local/bin
	install -s $(STUD)/$(STUD_TARGET) /usr/local/bin/stud
	-useradd haproxy
	-useradd stud
	-useradd redis
	-useradd mongo
	-useradd web
	cp -a runit/* /etc/service

uninstall:
	rm -fr /usr/local/bin/haproxy /usr/local/bin/stud /usr/local/bin/redis-* /usr/local/bin/mongo* /usr/local/bin/bsondump /etc/service/haproxy /etc/service/stud /etc/service/redis /etc/service/mongo
	-userdel web
	-userdel mongo
	-userdel redis
	-userdel stud
	-userdel haproxy

clean:
	rm -fr $(HAPROXY) $(STUD) $(REDIS) $(MONGO) $(WEBFS)

.PHONY: all check bin lib install uninstall clean
