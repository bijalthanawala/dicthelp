dicthelp: gnrcheap.o dicthelp.o
	gcc -o $@ gnrcheap.o dicthelp.o

dicthelp.o: dicthelp.c
	gcc -c dicthelp.c

gnrcheap.o: gnrcheap.c
	gcc -c gnrcheap.c

clean:
	rm gnrcheap.o dicthelp.o dicthelp 
