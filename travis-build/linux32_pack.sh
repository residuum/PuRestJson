#!/bin/bash

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python create-manual.py

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make machine=i386 deken"
cp ${CHROOTDIR}/PuRestJson/*.tar.gz .
