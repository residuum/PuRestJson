#!/bin/bash

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make machine=i386 deken"
cp ${CHROOTDIR}/PuRestJson/*.tar.gz .
