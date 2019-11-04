#!/bin/bash

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python create-manual.py ${CHROOTDIR}/PuRestJson/manual/

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make machine=armv7 deken.bits=32 deken"
cp ${CHROOTDIR}/PuRestJson/*.dek .
