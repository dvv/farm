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
LIGHTTPD=lighttpd-1.4.29
NGINX=nginx-1.1.4
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
#bin: $(ZEROMQ)/src/.libs/libzmq.a
#bin: $(MONGO)/bin/mongo $(HAPROXY)/haproxy $(STUD)/$(STUD_TARGET) $(REDIS)/src/redis-server $(RUNIT)/runsvdir $(LIGHTTPD)/src/lighttpd
#bin: $(LIGHTTPD)/src/lighttpd
bin: $(HAPROXY)/haproxy
#bin: $(NGINX)/src/nginx
#bin: $(STUD)/$(STUD_TARGET)

$(HAPROXY)/haproxy: $(HAPROXY)
	make -C $^ TARGET=generic

$(HAPROXY):
	wget http://haproxy.1wt.eu/download/1.5/src/devel/$(HAPROXY).tar.gz -O - | tar -xzpf -
	(cd $@ ; patch -Np1 < ../flash-policy.diff )

$(STUD)/$(STUD_TARGET): $(STUD)
	cp Makefile.stud $^
	make -C $^ -f Makefile.stud $(STUD_TARGET)

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

$(ZEROMQ)/src/.libs/libzmq.a: $(ZEROMQ)
	( cd $^ ; ./configure --prefix=/usr --enable-shared=no )
	make -C $^
	touch -c $@

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

$(LIGHTTPD)/src/lighttpd: $(LIGHTTPD)
	# TODO: need apt-get install libssl-dev
	( cd $^ ; ./configure --prefix=/usr/local --sbindir=/usr/local/bin --libdir=/usr/local/lib/lighttpd --without-pcre --without-bzip2 --with-openssl )
	make -C $^
	touch -c $@

$(LIGHTTPD):
	wget http://download.lighttpd.net/lighttpd/releases-1.4.x/$(LIGHTTPD).tar.gz -O - | tar -xzpf -

$(NGINX)/src/nginx: $(NGINX)
	# TODO: need apt-get install libssl-dev libpcre3-dev
	( cd $^ ; ./configure --prefix=/usr/local --sbin-path=/usr/local/bin --conf-path=/etc/service/nginx/conf --error-log-path=/dev/stderr --pid-path=/etc/service/nginx/.pid --lock-path=/etc/service/nginx/.lock --with-http_ssl_module --without-http_rewrite_module )
	make -C $^
	touch -c $@

$(NGINX):
	wget http://nginx.org/download/$(NGINX).tar.gz -O - | tar -xzpf -

install: bin
	install -s $(HAPROXY)/haproxy $(REDIS)/src/redis-server $(REDIS)/src/redis-cli /usr/local/bin
	install $(MONGO)/bin/* /usr/local/bin
	install -s $(STUD)/$(STUD_TARGET) /usr/local/bin/stud
	install -s $(NGINX)/objs/nginx /usr/local/bin
	#install -s $(RUNIT)/runsvdir $(RUNIT)/runsv $(RUNIT)/sv $(RUNIT)/chpst $(RUNIT)/svlogd /usr/local/bin
	make -C $(LIGHTTPD) install
	-useradd haproxy
	-useradd stud
	-useradd redis
	-useradd mongo
	-useradd web
	mkdir -p /etc/service
	cp -a service/* /etc/service

uninstall:
	make -C $(LIGHTTPD) uninstall
	#rm -fr /usr/local/bin/runsvdir /usr/local/bin/runsv /usr/local/bin/sv /usr/local/bin/chpst /usr/local/bin/svlogd
	rm -fr /usr/local/bin/haproxy /usr/local/bin/stud /usr/local/bin/nginx /usr/local/bin/redis-* /usr/local/bin/mongo* /usr/local/bin/bsondump /etc/service/farm /etc/service/redis /etc/service/mongo
	-userdel web
	-userdel mongo
	-userdel redis
	-userdel stud
	-userdel haproxy

clean:
	rm -fr $(HAPROXY) $(STUD) $(REDIS) $(MONGO) $(RUNIT) $(LIGHTTPD) $(NGINX)

.PHONY: all check bin lib install uninstall clean
