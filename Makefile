lib.name = purest_json

OBJECTS = rest.c oauth.c json-decode.c json-encode.c urlparams.c 
PDOBJECTS = 
EXAMPLES = purest-json-test.pd the-sound-of-money.pd twitter-client.pd binary-test.pd
EXTRA_DIST = README.md LICENSE.txt Changelog.txt test.json
HELPPATCHES = json-help.pd urlparams-help.pd rest-help.pd
UNITTESTS = 

datafiles = $(EXTRA_DIST) $(HELPPATCHES)
examplefiles = $(addprefix examples/, $(EXAMPLES))
class.sources = $(addprefix src/, $(OBJECTS))
uthash = src/uthash/src

cflags += -DVERSION='"$(lib.version)"' -I"$(uthash)" -std=c99 
ldflags = 
ldlibs = -lcurl -ljson-c -loauth

define forWindows
    ldlibs += -lpthread -lm -lwldap32 -lssl -lssh2 -lgcrypt -lgpg-error \
			  -lcrypto -lws2_32 -lgdi32 -lcrypt32 -lz -lidn2 -lunistring \
			  -latomic -lintl -liconv -lcharset
    cflags += -mthreads -DCURL_STATICLIB
    datafiles += cacert.pem
endef

define forDarwin
    datafiles += cacert.pem *.dylib
endef

lib.setup.sources = src/purest_json.c

# creating deken package
deken.bits=32
deken.ext=dek
deken.pack=zip -9 -r
deken.file= $(lib.name)[v$(lib.version)]($(system)-$(machine)-$(deken.bits))
deken.tmp=deken-tmp
deken.folder=$(lib.name)

PDLIBBUILDER_DIR=pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

deken:
	mkdir -p "$(deken.tmp)"
	mkdir "$(deken.tmp)/$(deken.folder)"
	cp $(executables) "$(deken.tmp)/$(deken.folder)/"
	cp $(datafiles) "$(deken.tmp)/$(deken.folder)/"
	mkdir "$(deken.tmp)/$(deken.folder)/examples"
	cp -r manual "$(deken.tmp)/$(deken.folder)/"
	cp $(examplefiles) "$(deken.tmp)/$(deken.folder)/examples"
	cd "$(deken.tmp)"; \
	  $(deken.pack) "$(deken.file).$(deken.ext)" "$(deken.folder)"; \
	  rm -rf "$(deken.folder)"; \
	  mv "$(deken.file).$(deken.ext)" ..; \
