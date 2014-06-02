#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/xmem.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/xmem.so"

ifndef PREFIX
  PREFIX = /usr/local/
endif

all: lib

lib:
	$(CC) -Wall -fopenmp -I. -fPIC -shared -c api.c
	$(CC) -Wall -fopenmp -I. -fPIC -shared -o libxmem.so api.o xmem.c -ldl

clean:
	rm -f *.so *.o  test

test:
	$(CC) -o test test.c -ldl

install:
	mkdir -p $(PREFIX)/bin $(PREFIX)/lib
	cat xmem | sed -e "s%FLEXMEM_HOME=$$%FLEXMEM_HOME=${PREFIX}%" > $(PREFIX)/bin/xmem
	chmod +x $(PREFIX)/bin/xmem
	cp libxmem.so $(PREFIX)/lib

uninstall:
	rm -f $(PREFIX)/bin/xmem
	rm -f $(PREFIX)/lib/libxmem.so
