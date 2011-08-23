default: couchpdb

couchpdb-test: couchpdb
	pdextended -stderr -lib couchpdb couchpdb-test.pd

couchpdb:
	gcc -Wall -shared -ansi -O2 -fPIC -lcurl -I/home/thomas/dev/diverse/Pd-0.42.5-extended/pd/src src/couchpdb.c -o couchpdb.pd_linux
