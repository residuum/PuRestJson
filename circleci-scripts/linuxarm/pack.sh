#!/bin/bash
set -e

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && bash ./linux_dependencies.sh *.pd_linux"

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/
python3 create-manual.py ${CHROOTDIR}/PuRestJson/manual/

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make machine=armv7 deken.bits=32 deken"
cp ${CHROOTDIR}/PuRestJson/*.dek .
