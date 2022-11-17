#!/bin/bash
set -e

sudo apt -y update
sudo apt -y install puredata-dev libjson-c-dev libcurl4-openssl-dev liboauth-dev zip patchelf

pip3 install --upgrade pip
pip3 install beautifulsoup4 lxml grip --user
