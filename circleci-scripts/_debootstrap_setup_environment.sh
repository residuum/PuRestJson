#!/bin/bash
set -e

sudo apt -y update
sudo apt -y install qemu-user-static debootstrap
sudo mkdir -p ${CHROOTDIR}
sudo qemu-debootstrap \
	--variant=buildd \
	--include=build-essential \
	--arch=${DEBIAN_ARCH} \
	${DIST} ${CHROOTDIR} http://deb.debian.org/debian
	
sudo rsync -a . ${CHROOTDIR}/PuRestJson 

echo deb http://deb.debian.org/debian \
    $DIST main contrib non-free \
    | sudo tee -a ${CHROOTDIR}/etc/apt/sources.list

sudo chroot ${CHROOTDIR} bash -c "apt -y update"
sudo chroot ${CHROOTDIR} bash -c "apt install -qq -y build-essential \
	puredata-dev libjson-c-dev libcurl4-openssl-dev liboauth-dev zip patchelf"

pip3 install --upgrade pip
pip3 install beautifulsoup4 lxml grip --user
