#! /bin/bash

OWNCLOUD_URL=https://purest_json:${OC_PASSWORD}@ssl-account.com/cloud.residuum.org/remote.php/dav/files/purest_json/travis

DEKENFILE=`ls *.dek`
if [ -n "$TRAVIS_TAG" ]; then
	BUILDFILE=${DEKENFILE}
else
	BUILDFILE=${TRAVIS_BUILD_NUMBER}_`date +%Y-%m-%d`_${DEKENFILE}
	mv "${DEKENFILE}" "${BUILDFILE}"
fi
curl -g -X PUT "${OWNCLOUD_URL}/${BUILDFILE}" --data-binary @"${BUILDFILE}"
