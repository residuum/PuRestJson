#!/bin/bash

MXE_DIR=/usr/lib/mxe
MXE_GCC=${MXE_DIR}/usr/bin/x86_64-w64-mingw32.static-gcc
PD_DIR=/tmp/pd

make \
  system=Windows \
  machine=x86_64 \
  uname=MINGW \
  pdincludepath=${PD_DIR}/src \
  pdbinpath=${PD_DIR}/bin \
  CC=${MXE_GCC} \
  arch.c.flags='-march=core2 -mfpmath=sse -msse -msse2 -msse3 \
    -I "/usr/lib/mxe/usr/x86_64-w64-mingw32.static/include"' \
  ldflags='-L "/usr/lib/mxe/usr/x86_64-w64-mingw32.static/lib"'
