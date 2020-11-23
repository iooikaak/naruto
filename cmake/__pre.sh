#!/bin/sh
#$1:src file, $2:header path, $3:output file
gcc -C -E $1 $2 -o $3