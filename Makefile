all: client.o server server.o rclient1.o rclient2.o rclient1 rclient2 

client.o: 
	gcc -g -Wall -c client.c 

server.o: 
	gcc -g -Wall -c server.c

server: 
	gcc -g -Wall server.c -o server

rclient1.o:
	gcc -g -Wall -c rclient1.c

rclient1: client.o
	gcc -g -Wall rclient1.c -o rclient1 client.o

rclient2.o: 
	gcc -g -Wall -c rclient2.c

rclient2: client.o
	gcc -g -Wall rclient2.c -o rclient2 client.o

clean:
	rm client client.o server server.o rclient1 rclient1.o rclient2 rclient2.o

submission:
	tar czvf prog5.tgz Makefile client.c server.c rclient1.c rclient2.c README rpcdefs.h
