#!/bin/bash

sudo apt-get -y install qemu-user-static debootstrap
sudo mkdir -p ${CHROOTDIR}
sudo qemu-debootstrap \
	--variant=buildd \
	--include=build-essential \
	--arch=${DEBIAN_ARCH} \
	${DIST} ${CHROOTDIR} http://deb.debian.org/debian
	
sudo rsync -a ${TRAVIS_BUILD_DIR} ${CHROOTDIR}/ 

echo deb http://deb.debian.org/debian \
    $DIST main contrib non-free \
    | sudo tee -a ${CHROOTDIR}/etc/apt/sources.list

sudo chroot ${CHROOTDIR} bash -c "apt-get update"
sudo chroot ${CHROOTDIR} bash -c "apt-get install -qq -y build-essential \
	puredata-dev libjson-c-dev libcurl4-nss-dev liboauth-dev"

pip install grip beautifulsoup4 lxml --user
