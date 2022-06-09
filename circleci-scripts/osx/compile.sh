#!/bin/bash
set -e

make arch="x86_64 arm64" \
	pdincludepath=/tmp/Pd-${PDVERSION}-macosx7.app/Contents/Resources/src
