#!/bin/bash

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
    xz-utils \
	grip

wget --no-verbose -O /tmp/mxe.tar.bz2 \
	https://purest_json:${OC_PASSWORD}@cloud.residuum.org/remote.php/webdav/mxe.tar.bz2
mkdir /tmp/mxe
tar -xf /tmp/mxe.tar.bz2 -C /tmp/mxe

wget --no-verbose -O /tmp/pd.zip "${PD_URL}"
unzip -q -d /tmp /tmp/pd.zip

PATH=$PATH:/tmp/usr/bin

pip3 install --upgrade pip
pip3 install grip beautifulsoup4 lxml --user
