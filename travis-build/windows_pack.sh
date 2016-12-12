#!/bin/bash

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python create-manual.py

wget https://curl.haxx.se/ca/cacert.pem

make \
	machine=i386 \
	system=Windows \
	deken
