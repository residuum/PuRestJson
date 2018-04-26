#!/bin/bash

git clone https://github.com/residuum/PuRestJson.wiki.git /tmp/PuRestJson.wiki/

python create-manual.py

wget https://curl.haxx.se/ca/cacert.pem

make \
	machine=${MACHINE} \
	system=Windows \
	deken.bits=${BITS} \
	deken
