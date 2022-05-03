#!/bin/bash

export PD_UNZIP_PATH=/tmp/pd-${PDVERSION}-i386
export PD_URL=http://msp.ucsd.edu/Software/pd-${PDVERSION}-i386.msw.zip
bash ./circleci-scripts/_win_setup_environment.sh
