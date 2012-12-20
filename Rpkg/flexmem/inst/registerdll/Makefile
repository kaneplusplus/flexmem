#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/flexmem.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/flexmem.so"

sharedobject: flexmemobject
	g++ -shared flexmem.o -o libflexmem.so -ldl 

flexmemobject:
	g++ -I../include -fPIC -o2 -c -o flexmem.o flexmem.cpp

clean:
	rm -f *.so *.o 

install:
	cp libflexmem.so /usr/local/lib/ && cp flexmem /usr/local/bin/
