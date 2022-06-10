#!/bin/bash
set -e

make arch="x86_64 arm64" \
	pdincludepath=/Volumes/Pd-${PDVERSION}/Pd-${PDVERSION}.app/Contents/Resources/src
