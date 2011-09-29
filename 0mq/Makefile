all: bin

bin: relay

relay: relay.c
	gcc -o $@ $^ -lzmq
	strip -s $@

.PHONE: all bin
