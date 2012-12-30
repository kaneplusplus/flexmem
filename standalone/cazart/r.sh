#!/bin/bash
export LD_PRELOAD=$(pwd)/libflexmem.so
exec R 
