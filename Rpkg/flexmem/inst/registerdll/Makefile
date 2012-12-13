#Linux: sharedobject
#	export LD_PRELOAD=$(pwd)/flexmem.so

#Mac: sharedobject
#	export DYLD_INSERT_LIBRARIES="$(PWD)/flexmem.so"

sharedobject: flexmem 
	g++ -shared flexmem.o -o libflexmem.so -ldl 

flexmem:
	g++ -I/opt/local/include -I../include -fPIC -g -c -o flexmem.o flexmem.cpp

clean:
	rm -f *.so *.o 

install:
	mv libflexmem.so /usr/local/lib/
