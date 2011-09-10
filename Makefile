CC_LINUX = gcc 
CC_MINGW = i686-w64-mingw32-gcc 

FLAGS_LINUX = -Wall -fPIC -shared -ansi -O2 
FLAGS_MINGW = -Wall -shared -ansi -O2 

LIB_LINUX = -lcurl -ljson
LIB_MINGW = -lcurl -ljson

COUCHPDB_LINUX = bin/couchpdb.pd_linux
COUCHPDB_MINGW = bin/couchpdb.dll

PD_EXE = pdextended

PDINCLUDE = -I../../../diverse/Pd-0.42.5-extended/pd/src

COUCHPDB_SRC = src/couchpdb.c

default: couchpdb-linux 

all: couchpdb-linux couchpdb-mingw doc

couchpdb-test: couchpdb
	$(PD_EXE) -stderr -lib bin/couchpdb couchpdb-test.pd

couchpdb-linux:
	$(CC_LINUX) $(FLAGS_LINUX) $(LIB_LINUX) $(PDINCLUDE) $(COUCHPDB_SRC) -o $(COUCHPDB_LINUX) 

couchpdb-mingw:
	$(CC_MINGW) $(FLAGS_MINGW) $(LIB_MINGW) $(PDINCLUDE) $(COUCHPDB_SRC) -o $(COUCHPDB_MINGW) 

doc:
	doxygen Doxyfile
