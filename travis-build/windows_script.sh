#!/bin/bash

MXE_DIR=/usr/lib/mxe
MXE_GCC=${MXE_DIR}/usr/bin/i686-w64-mingw32.static-gcc
PD_DIR=/tmp/pd

make \
  system=Windows \
  machine=i386 \
  uname=MINGW \
  pdincludepath=${PD_DIR}/src \
  pdbinpath=${PD_DIR}/bin \
  CC=${MXE_GCC} \
  arch.c.flags='-march=pentium4 -msse -msse2 -mfpmath=sse -I \
    "/usr/lib/mxe/usr/i686-w64-mingw32.static/include"' \
  ldflags='-L "/usr/lib/mxe/usr/i686-w64-mingw32.static/lib"'
