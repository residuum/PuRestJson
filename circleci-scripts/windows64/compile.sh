#!/bin/bash
set -e

export PD_UNZIP_PATH=/tmp/pd-${PDVERSION}
bash ./circleci-scripts/_win_compile.sh
