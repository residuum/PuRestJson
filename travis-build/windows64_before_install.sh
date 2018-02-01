#!/bin/bash

MXE_TARGET=x86-64-w64-mingw32.static
PD_PATH_STRING=pd-0.48-1test2-ia64
PD_DIR=/tmp/pd

echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
    | sudo tee /etc/apt/sources.list.d/mxeapt.list
sudo apt-key adv --keyserver x-hkp://keys.gnupg.net \
    --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB
sudo apt-get update
sudo apt-get -y install mxe-${MXE_TARGET}-curl mxe-${MXE_TARGET}-json-c \
	mxe-${MXE_TARGET}-liboauth mxe-${MXE_TARGET}-pthreads

wget -O /tmp/pd.zip http://msp.ucsd.edu/Software/${PD_PATH_STRING}.msw.zip
unzip -d /tmp /tmp/pd.zip
mv /tmp/${PD_PATH_STRING} /tmp/pd

pip install grip beautifulsoup4 lxml
