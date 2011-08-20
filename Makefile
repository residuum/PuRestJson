default: couchdb

couchdb-test: couchdb
	pdextended -stderr couchdb-test.pd

couchdb:
	gcc -Wall -shared -ansi -O2 -fPIC -lcurl -I/home/thomas/dev/diverse/Pd-0.42.5-extended/pd/src couchdb.c -o couchdb.pd_linux
