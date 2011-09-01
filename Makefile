PLATFORM = linux
#PLATFORM = macosx
#PLATFORM = windows 

PD_EXE = pdextended

PDINCLUDE = -I$(HOME)/dev/diverse/Pd-0.42.5-extended/pd/src

COUCHPDB_linux = bin/couchpdb.pd_linux
COUCHPDB_macosx = bin/couchpdb.pd_darwin
COUCHPDB_windows = bin/couchpdb.dll

COUCHPDB_SRC = src/couchpdb.c

CC_linux = gcc -Wall

default: couchpdb

couchpdb-test: couchpdb
	$(PD_EXE) -stderr -lib bin/couchpdb couchpdb-test.pd

couchpdb:
	$(CC_$(PLATFORM)) -shared -ansi -O2 -fPIC -lcurl -ljson $(PDINCLUDE) $(COUCHPDB_SRC) -o $(COUCHPDB_$(PLATFORM)) 
