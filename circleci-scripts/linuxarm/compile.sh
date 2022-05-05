#!/bin/bash
set -e

sudo chroot ${CHROOTDIR} bash -c "cd PuRestJson && make"
