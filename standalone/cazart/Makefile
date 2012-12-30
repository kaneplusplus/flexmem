#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/flexmem.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/flexmem.so"

all: lib test

lib:
	gcc -Wall -fopenmp -I. -fPIC -shared -o libflexmem.so flexmem.c -Wl,-E -ldl

clean:
	rm -f *.so *.o  test

test:
	$(CC) -shared -o testlib.so testlib.c
	$(CC) -o test test.c -ldl
