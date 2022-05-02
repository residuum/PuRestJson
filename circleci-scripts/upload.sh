#! /bin/bash

OWNCLOUD_URL=https://purest_json:${OC_PASSWORD}@cloud.residuum.org/remote.php/dav/files/purest_json/travis

DEKENFILE=`ls *.dek`
if [ -n "$CIRCLE_TAG" ]; then
	BUILDFILE=${DEKENFILE}
else
	BUILDFILE=${CIRCLE_BUILD_NUM}_`date +%Y-%m-%d`_${DEKENFILE}
	mv "${DEKENFILE}" "${BUILDFILE}"
fi
curl -g -X PUT "${OWNCLOUD_URL}/${BUILDFILE}" --data-binary @"${BUILDFILE}"
