#!/bin/bash
set -e

git submodule init
git submodule update

export PATH="$PATH:~/.local/bin:/Users/distiller/Library/Python/3.9/bin"

SECONDS=0
bash ./circleci-scripts/${SYSTEM}/setup_environment.sh
echo "Time for setting up environment: ${SECONDS}s"
SECONDS=0
bash ./circleci-scripts/${SYSTEM}/compile.sh
echo "Time for compilation: ${SECONDS}s"
SECONDS=0
bash ./circleci-scripts/${SYSTEM}/pack.sh
echo "Time for packing: ${SECONDS}s"
SECONDS=0
bash ./circleci-scripts/upload.sh
echo "Time for upload: ${SECONDS}s"
