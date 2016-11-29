#!/bin/bash

ls ${CHROOTDIR}

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make"

