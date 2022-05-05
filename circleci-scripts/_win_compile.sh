#!/bin/bash
set -e

MXE_DIR=/tmp/mxe
MXE_GCC=${MXE_DIR}/usr/bin/${MXE_PATH_SLUG}-gcc

make \
  system=Windows \
  machine=${MACHINE} \
  uname=MINGW \
  pdincludepath=${PD_UNZIP_PATH}/src \
  pdbinpath=${PD_UNZIP_PATH}/bin \
  CC=${MXE_GCC} \
  arch.c.flags='${CUSTOM_CFLAGS} \
    -I "${MXE_DIR}/usr/${MXE_PATH_SLUG}/include"' \
  ldflags='-L "${MXE_DIR}/usr/${MXE_PATH_SLUG}/lib"'
