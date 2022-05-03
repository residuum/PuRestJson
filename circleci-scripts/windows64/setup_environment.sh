#!/bin/bash

export PD_UNZIP_PATH=/tmp/pd-${PDVERSION}
export PD_URL=http://msp.ucsd.edu/Software/pd-${PDVERSION}.msw.zip
bash ./circleci-scripts/_win_setup_environment.sh
