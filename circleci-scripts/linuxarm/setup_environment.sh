#!/bin/bash

DIST=stretch
DEBIAN_ARCH=armhf
CHROOTDIR=/opt/crosscompilation

bash ./circleci-scripts/_debootstrap_setup_environment.sh
