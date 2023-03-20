#CC=/home/jschuchart/opt/gcc-12.2.0/bin/gcc
CC=gcc

all: create
#libgcc create
    #orc

create: create_memcpy.c
	$(CC) -o create -O3 create_memcpy.c -lgccjit

test: test.c
	$(CC) -o test -O3 test.c -lgccjit

libgcc: libgcc.c
	$(CC) -o libgcc -O3 libgcc.c -lgccjit 
#-I/home/yli137/opt/gcc-12/include -L/home/yli137/opt/gcc-12/lib -lgccjit

clean:
	rm libgcc create
