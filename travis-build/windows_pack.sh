#!/bin/bash

wget https://curl.haxx.se/ca/cacert.pem

make \
	machine=i386 \
	system=Windows \
	deken
