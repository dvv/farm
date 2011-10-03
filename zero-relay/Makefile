all: bin

bin: zero-relay

zero-relay: zero-relay.c
	gcc -o $@ $^ -lzmq -luuid -lstdc++ -lpthread
	strip -s $@

.PHONE: all bin
