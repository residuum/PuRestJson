CC_LINUX = gcc 
CC_MINGW = i686-w64-mingw32-gcc 

FLAGS_LINUX = -Wall -fPIC -shared -ansi -O2 
FLAGS_MINGW = -Wall -static -ansi -O2 

LIB_LINUX = -lcurl -ljson
LIB_MINGW = -lcurl -ljson

PUREST_JSON_LINUX = bin/purest_json.pd_linux
PUREST_JSON_MINGW = bin/purest_json.dll

PD_EXE = pdextended

PDINCLUDE = -I../../../diverse/Pd-0.42.5-extended/pd/src

PUREST_JSON_SRC = src/purest_json.c

default: purest-json-linux 

all: purest-json-linux purest-json-mingw doc

test: purest-json-linux
	$(PD_EXE) -stderr -lib bin/purest_json purest-json-test.pd

debug: purest-json-linux
	gdb --args $(PD_EXE) -stderr -lib bin/purest_json purest-json-test.pd

purest-json-linux:
	$(CC_LINUX) $(FLAGS_LINUX) $(LIB_LINUX) $(PDINCLUDE) $(PUREST_JSON_SRC) -o $(PUREST_JSON_LINUX) 

purest-json-mingw:
	PATH=/usr/local/mingw/bin:$(PATH)
	CPATH=/usr/local/mingw/include
	LD_LIBRARY_PATH=/usr/local/mingw/lib
	PKG_CONFIG_DIR=/usr/local/mingw/lib/pkgconfig
	$(CC_MINGW) $(FLAGS_MINGW) $(LIB_MINGW) $(PDINCLUDE) $(PUREST_JSON_SRC) -o $(PUREST_JSON_MINGW) 
