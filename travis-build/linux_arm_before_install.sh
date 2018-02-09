#!/bin/bash

DIST=stretch
DEBIAN_ARCH=armhf
CHROOTDIR=/opt/crosscompilation

bash ./travis-build/_debootstrap_before_install.sh
