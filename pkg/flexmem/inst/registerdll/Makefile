Linux: sharedobject
	export LD_PRELOAD=$(pwd)/flexmem.so

Mac: sharedobject
	export DYLD_INSERT_LIBRARIES="$(PWD)/flexmem.so"

flexmem:
	g++ -I/opt/local/include -I../include -fPIC -g -c -o flexmem.o flexmem.cpp

sharedobject: flexmem 
	g++ -shared flexmem.o -o flexmem.so -ldl 

clean:
	rm -f *.so *.o 
