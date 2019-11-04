#!/bin/bash

#echo "deb http://pkg.mxe.cc/repos/apt bionic main" \
#    | sudo tee /etc/apt/sources.list.d/mxeapt.list
#sudo apt-key adv --keyserver keyserver.ubuntu.com \
#	--recv-keys C6BF758A33A3A276
#sudo apt-get update
#sudo apt-get -y install mxe-${MXE_TARGET}-curl mxe-${MXE_TARGET}-json-c \
#	mxe-${MXE_TARGET}-liboauth mxe-${MXE_TARGET}-pthreads

sudo apt-get update
sudo apt-get install \
    autoconf \
    automake \
    autopoint \
    bash \
    bison \
    bzip2 \
    flex \
    g++ \
    g++-multilib \
    gettext \
    git \
    gperf \
    intltool \
    libc6-dev-i386 \
    libgdk-pixbuf2.0-dev \
    libltdl-dev \
    libssl-dev \
    libtool-bin \
    libxml-parser-perl \
    lzip \
    make \
    openssl \
    p7zip-full \
    patch \
    perl \
    pkg-config \
    python \
    ruby \
    sed \
    unzip \
    wget \
    xz-utils

wget --no-verbose -O /tmp/mxe.tar.bz2 \
	https://purest_json:${OC_PASSWORD}@ssl-account.com/cloud.residuum.org/remote.php/webdav/mxe.tar.bz2
mkdir /tmp/mxe
tar -xf /tmp/mxe.tar.bz2 -C /tmp/mxe
 
wget --no-verbose -O /tmp/pd.zip "${PD_URL}"
unzip -q -d /tmp /tmp/pd.zip

if [ "$PD_DIR" != "/tmp/$PD_UNZIP_PATH" ]; then
	echo "Move unzipped files"
	mv /tmp/${PD_UNZIP_PATH} ${PD_DIR}
fi

pip install grip beautifulsoup4 lxml --user
