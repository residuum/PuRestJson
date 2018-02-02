#!/bin/bash

DIST=trusty

sudo apt-get -y install debootstrap
sudo mkdir -p ${CHROOTDIR}
sudo debootstrap \
	--variant=buildd \
	--include=build-essential \
	--arch=i386 \
	${DIST} ${CHROOTDIR} http://archive.ubuntu.com/ubuntu/
	
sudo rsync -a ${TRAVIS_BUILD_DIR} ${CHROOTDIR}/ 

echo deb http://archive.ubuntu.com/ubuntu/ \
    $DIST restricted universe multiverse \
    | sudo tee -a ${CHROOTDIR}/etc/apt/sources.list

sudo chroot ${CHROOTDIR} bash -c "apt-get update"
sudo chroot ${CHROOTDIR} bash -c "apt-get install -qq -y build-essential \
	puredata-dev libjson-c-dev libcurl4-nss-dev liboauth-dev"

pip install grip beautifulsoup4 lxml --user
