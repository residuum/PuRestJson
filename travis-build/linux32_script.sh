#!/bin/bash

ls /usr

ls /usr/bin/*gcc*

make \
	CC='gcc -m32' \
	arch.c.flags='-march=pentium4 -mfpmath=sse -msse -msse2' \
	arch.ld.flags='-L "/usr/libx32" -L "/libx32"'
