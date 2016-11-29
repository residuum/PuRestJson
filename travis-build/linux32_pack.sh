#!/bin/bash

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make deken"
cp ${CHROOTDIR}/PuRestJson/*.tar.gz .
