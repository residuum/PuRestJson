#! /bin/bash

DEKENFILE=`ls *.tar.gz *.zip`

if [ -n "$TRAVIS_TAG" ]; then
	echo "Tag build"
	echo "directly uploading ${DEKENFILE} to puredata.info"
else
	BUILDFILE=${TRAVIS_BUILD_NUMBER}_`date +%Y-%m-%d`_${DEKENFILE}
	mv ${DEKENFILE} ${BUILDFILE}
	echo "Commit build"
	echo "uploading ${BUILDFILE} to some staging system"
fi
