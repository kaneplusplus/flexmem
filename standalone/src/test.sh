#!/bin/bash
ulimit -c 100000
export LD_PRELOAD=$(pwd)/libflexmem.so
exec ./test
