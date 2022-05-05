#!/bin/bash
set -e

make \
	pdincludepath=/tmp/Pd-${PDVERSION}-macosx7.app/Contents/Resources/src
