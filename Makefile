PLATFORM = linux
#PLATFORM = macosx
#PLATFORM = windows 

PD_EXE = pdextended

PDINCLUDE = -I$(HOME)/dev/diverse/Pd-0.42.5-extended/pd/src

COUCHPDB_linux = couchpdb.pd_linux
COUCHPDB_macosx = couchpdb.pd_darwin
COUCHPDB_windows = couchpdb.dll

COUCHPDB_SRC = src/couchpdb.c

CC_linux = gcc -Wall

default: couchpdb

couchpdb-test: couchpdb
	$(PD_EXE) -stderr -lib couchpdb couchpdb-test.pd

couchpdb:
	$(CC_$(PLATFORM)) -shared -ansi -O2 -fPIC -lcurl -ljson $(PDINCLUDE) $(COUCHPDB_SRC) -o $(COUCHPDB_$(PLATFORM)) 
