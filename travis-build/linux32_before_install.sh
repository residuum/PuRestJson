#!/bin/bash

sudo dpkg --add-architecture i386

sudo apt-get update

sudo apt-get -y install puredata-dev libjson-c-dev libcurl4-nss-dev liboauth-dev \
	puredata-core:i386 libjson-c2:i386 libcurl3-nss:i386 liboauth0:i386 \
	libc6-dev-i386 gcc-multilib
