#!/bin/bash
set -e

git submodule init
git submodule update

export PATH="$PATH:~/.local/bin:/Users/distiller/Library/Python/3.9/bin"

bash ./circleci-scripts/${SYSTEM}/setup_environment.sh
bash ./circleci-scripts/${SYSTEM}/compile.sh
bash ./circleci-scripts/${SYSTEM}/pack.sh
bash ./circleci-scripts/upload.sh
