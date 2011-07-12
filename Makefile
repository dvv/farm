#
# build haproxy and stunnel
#
# dronnikov@gmail.com 2011
#

HAPROXY=haproxy-1.5-dev6
STUD=stud-latest

all: bin

bin: $(HAPROXY)/haproxy $(STUD)/stud

$(HAPROXY)/haproxy: $(HAPROXY)
	make -C $^ TARGET=linux26

$(HAPROXY): $(HAPROXY).tar.gz
	tar xzpf $^

$(HAPROXY).tar.gz:
	wget http://haproxy.1wt.eu/download/1.5/src/devel/$(HAPROXY).tar.gz

$(STUD)/stud: $(STUD)
	dpkg -s libssl-dev
	dpkg -s libev-dev
	make -C $^

$(STUD): $(STUD).tar.gz
	tar xzpf $^
	mv bumptech* $@

$(STUD).tar.gz:
	wget https://github.com/bumptech/stud/tarball/master -O $@

install: bin
	sudo dpkg -s runit
	sudo install -s $(HAPROXY)/haproxy $(STUD)/stud /usr/local/bin
	sudo cp -a runit/* /etc/service
	-sudo useradd haproxy
	-sudo useradd stud

uninstall:
	rm -fr /usr/local/bin/haproxy /usr/local/bin/stud /etc/service/haproxy /etc/service/stud
	-userdel stud
	-userdel haproxy

clean:
	rm -fr $(HAPROXY) $(STUD)

distclean: clean
	rm -fr $(HAPROXY).tar.gz $(STUD).tar.gz

.PHONY: all bin lib install uninstall clean distclean
