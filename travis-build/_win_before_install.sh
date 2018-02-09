#!/bin/bash

echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
    | sudo tee /etc/apt/sources.list.d/mxeapt.list
sudo apt-key adv --keyserver x-hkp://keys.gnupg.net \
    --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB
sudo apt-get update
sudo apt-get -y install mxe-${MXE_TARGET}-curl mxe-${MXE_TARGET}-json-c \
	mxe-${MXE_TARGET}-liboauth mxe-${MXE_TARGET}-pthreads

wget -O /tmp/pd.zip "${PD_URL}"
unzip -d /tmp /tmp/pd.zip
if [ "$PD_DIR" != "/tmp/$PD_UNZIP_PATH" ]; then
	echo "Move unzipped files"
	mv /tmp/${PD_UNZIP_PATH} ${PD_DIR}
fi

pip install grip beautifulsoup4 lxml --user
