all: master tcpscanner

master: master.c
	gcc -o $@ $^

tcpscanner: tcpscanner.c
	gcc -o $@ $^ -lpthread

