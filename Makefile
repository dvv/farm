#
# build haproxy and stunnel
#
# dronnikov@gmail.com 2011
#

HAPROXY=haproxy-1.5-dev6
STUD=stud-latest
REDIS=redis-latest
MONGO=mongodb-linux-i686-1.8.3

all: check bin

check:
	dpkg -s libssl-dev >/dev/null
	dpkg -s libev-dev >/dev/null
	dpkg -s runit >/dev/null
	dpkg -s ipsvd >/dev/null

bin: $(MONGO)/bin/mongo $(HAPROXY)/haproxy $(STUD)/stud $(REDIS)/src/redis-server

$(HAPROXY)/haproxy: $(HAPROXY)
	make -C $^ TARGET=linux26

$(HAPROXY): $(HAPROXY).tar.gz
	tar xzpf $^

$(HAPROXY).tar.gz:
	wget http://haproxy.1wt.eu/download/1.5/src/devel/$(HAPROXY).tar.gz

$(STUD)/stud: $(STUD)
	make -C $^

$(STUD): $(STUD).tar.gz
	tar xzpf $^
	#mv bumptech* $@
	mv dvv* $@

$(STUD).tar.gz:
	#wget https://github.com/bumptech/stud/tarball/master -O $@
	wget https://github.com/dvv/stud/tarball/master -O $@

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
	wget http://fastdl.mongodb.org/linux/$(MONGO).tgz -O $@

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
	rm -fr $(HAPROXY) $(STUD) $(REDIS) $(MONGO)
	rm -fr $(HAPROXY).tar.gz $(STUD).tar.gz $(REDIS).tar.gz $(MONGO).tgz

.PHONY: all check bin lib install uninstall clean
