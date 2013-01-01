#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/flexmem.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/flexmem.so"

ifndef PREFIX
  PREFIX = /usr/local/
endif

all: lib

lib:
	gcc -Wall -fopenmp -I. -fPIC -shared -o libflexmem.so flexmem.c -ldl

clean:
	rm -f *.so *.o  test

test:
	$(CC) -o test test.c -ldl

install:
	mkdir -p $(PREFIX)/bin $(PREFIX)/lib
	cat flexmem | sed -e "s%FLEXMEM_HOME=$$%FLEXMEM_HOME=${PREFIX}%" > $(PREFIX)/bin/flexmem
	chmod +x $(PREFIX)/bin/flexmem
	cp libflexmem.so $(PREFIX)/lib

uninstall:
	rm -f $(PREFIX)/bin/flexmem
	rm -f $(PREFIX)/lib/libflexmem.so
