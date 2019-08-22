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

git clone https://github.com/mxe/mxe /tmp/mxe
cd /tmp/mxe
travis_wait 30 make MXE_TARGETS=${MXE_PATH_SLUG} curl liboauth pthreads json-c

wget -O /tmp/pd.zip "${PD_URL}"
unzip -d /tmp /tmp/pd.zip
if [ "$PD_DIR" != "/tmp/$PD_UNZIP_PATH" ]; then
	echo "Move unzipped files"
	mv /tmp/${PD_UNZIP_PATH} ${PD_DIR}
fi

pip install grip beautifulsoup4 lxml --user
