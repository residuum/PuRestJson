#!/bin/bash


git submodule init
git submodule update

bash ./circleci-scripts/${SYSTEM}/setup_environment.sh
bash ./circleci-scripts/${SYSTEM}/compile.sh
bash ./circleci-scripts/${SYSTEM}/pack.sh
bash ./circleci-scripts/upload.sh