#!/bin/bash
export LD_PRELOAD=$(pwd)/libflexmem.so
./libflexmem.so
exec /usr/local/bin/R
