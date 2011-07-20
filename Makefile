#
# build haproxy and stunnel
#
# dronnikov@gmail.com 2011
#

HAPROXY=haproxy-1.5-dev6
STUD=stud-latest
REDIS=redis-latest

all: bin

bin: $(HAPROXY)/haproxy $(STUD)/stud $(REDIS)/src/redis-server

$(HAPROXY)/haproxy: $(HAPROXY)
	make -C $^ TARGET=linux26

$(HAPROXY): $(HAPROXY).tar.gz
	tar xzpf $^

$(HAPROXY).tar.gz:
	wget http://haproxy.1wt.eu/download/1.5/src/devel/$(HAPROXY).tar.gz

$(STUD)/stud: $(STUD)
	which dpkg && sudo dpkg -s libssl-dev || sudo emerge dev-libs/openssl
	which dpkg && sudo dpkg -s libev-dev || sudo emerge dev-libs/libev
	make -C $^

$(STUD): $(STUD).tar.gz
	tar xzpf $^
	mv bumptech* $@

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

install: bin
	sudo install -s $(HAPROXY)/haproxy $(STUD)/stud $(REDIS)/src/redis-server $(REDIS)/src/redis-cli /usr/local/bin
	-sudo useradd haproxy
	-sudo useradd stud
	-sudo useradd redis
	if test -d /service ; then \
		sudo cp -a runit/* /service ; \
	elif sudo dpkg -s runit ; \
		test -d /etc/service && sudo cp -a runit/* /etc/service ; \
	fi

uninstall:
	sudo rm -fr /usr/local/bin/haproxy /usr/local/bin/stud /usr/local/bin/redis-* /etc/service/haproxy /etc/service/stud /etc/service/redis
	-sudo userdel redis
	-sudo userdel stud
	-sudo userdel haproxy

clean:
	rm -fr $(HAPROXY) $(STUD) $(REDIS)

distclean: clean
	rm -fr $(HAPROXY).tar.gz $(STUD).tar.gz $(REDIS).tar.gz

.PHONY: all bin lib install uninstall clean distclean
