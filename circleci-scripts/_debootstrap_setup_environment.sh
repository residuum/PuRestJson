#!/bin/bash
set -e

sudo apt-get update
sudo apt-get -y install qemu-user-static debootstrap
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

sudo chroot ${CHROOTDIR} bash -c "apt-get update"
sudo chroot ${CHROOTDIR} bash -c "apt-get install -qq -y build-essential \
	puredata-dev libjson-c-dev libcurl4-openssl-dev liboauth-dev zip"

pip3 install --upgrade pip
pip3 install beautifulsoup4 lxml grip --user
