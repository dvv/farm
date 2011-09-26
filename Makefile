#
# build haproxy and stunnel
#
# dronnikov@gmail.com 2011
#

HAPROXY=haproxy-1.5-dev7
STUD=stud-latest
REDIS=redis-latest
ZEROMQ=zeromq-2.1.9
ZEROMQNODE=zeromq-0.5.1
RUNIT=runit-2.1.1
IPSVD=ipsvd-1.0.0
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

#bin: $(ZEROMQNODE)/binding.node
bin: $(MONGO)/bin/mongo $(HAPROXY)/haproxy $(STUD)/$(STUD_TARGET) $(REDIS)/src/redis-server $(RUNIT)/runsvdir $(IPSVD)/tcpsvd

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

busybox/busybox: busybox
	make -C $^ defconfig
	make -C $^

busybox:
	wget http://www.busybox.net/downloads/busybox-snapshot.tar.bz2 -O - | tar -xjpf -

$(RUNIT)/runsvdir: $(RUNIT)
	make -C $^

$(RUNIT):
	wget http://smarden.org/runit/$(RUNIT).tar.gz -O - | tar -xzpf -
	mv admin/$(RUNIT)/src $(RUNIT)
	rm -fr admin

$(IPSVD)/tcpsvd: $(IPSVD)
	make -C $^

$(IPSVD):
	wget http://smarden.org/ipsvd/$(IPSVD).tar.gz -O - | tar -xzpf -
	mv net/$(IPSVD)/src $(IPSVD)
	rm -fr net

$(ZEROMQ)/src/.libs/libzmq.a: $(ZEROMQ)
	( cd $^ ; ./configure --prefix=/usr )
	make -C $^
	touch -c $@
	sudo make install
	npm install zeromq

$(ZEROMQ):
	# TODO: need apt-get install uuid-dev
	wget http://download.zeromq.org/$(ZEROMQ).tar.gz -O - | tar -xzpf -

$(ZEROMQNODE)/binding.node: $(ZEROMQ)/src/.libs/libzmq.a $(ZEROMQNODE)
	#( cd $(ZEROMQNODE) ; PKG_CONFIG_PATH=../$(ZEROMQ)/src CXXFLAGS="" node-waf -vv configure build )
	#( cd $(ZEROMQNODE) ; g++ build/default/binding_1.o -o build/default/binding.node -I$(ROOT)/$(ZEROMQ)/include -L$(ROOT)/$(ZEROMQ)/src/.libs -lzmq -luuid -lrt -lpthread )
	# TODO: need apt-get install libzmq-dev
	( cd $(ZEROMQNODE) ; node-waf configure build )
	touch -c $@

$(ZEROMQNODE):
	wget http://registry.npmjs.org/zeromq/-/$(ZEROMQNODE).tgz -O - | tar -xzpf -
	mv package $@

install: bin
	install -s $(HAPROXY)/haproxy $(REDIS)/src/redis-server $(REDIS)/src/redis-cli /usr/local/bin
	install $(MONGO)/bin/* /usr/local/bin
	install -s $(STUD)/$(STUD_TARGET) /usr/local/bin/stud
	#install -s $(RUNIT)/runsvdir $(RUNIT)/runsv $(RUNIT)/sv $(RUNIT)/chpst $(RUNIT)/svlogd /usr/local/bin
	install -s $(IPSVD)/tcpsvd /usr/local/bin
	-useradd haproxy
	-useradd stud
	-useradd redis
	-useradd mongo
	-useradd web
	mkdir -p /etc/service
	cp -a service/* /etc/service

uninstall:
	rm -fr /usr/local/bin/tcpsvd
	#rm -fr /usr/local/bin/runsvdir /usr/local/bin/runsv /usr/local/bin/sv /usr/local/bin/chpst /usr/local/bin/svlogd
	rm -fr /usr/local/bin/haproxy /usr/local/bin/stud /usr/local/bin/redis-* /usr/local/bin/mongo* /usr/local/bin/bsondump /etc/service/haproxy /etc/service/stud /etc/service/redis /etc/service/mongo
	-userdel web
	-userdel mongo
	-userdel redis
	-userdel stud
	-userdel haproxy

clean:
	rm -fr $(HAPROXY) $(STUD) $(REDIS) $(MONGO) $(RUNIT) $(IPSVD)

.PHONY: all check bin lib install uninstall clean
