lib.name = purest_json

OBJECTS = rest oauth json-decode json-encode urlparams 
PDOBJECTS = 
EXAMPLES = purest-json-test.pd the-sound-of-money.pd statistics.pd twitter-client.pd binary-test.pd
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
    ldlibs += -lpthread -lm -lwldap32 -lgnutls -lhogweed -lgmp -lssl -lnettle \
			  -lssh2 -lgcrypt -lgpg-error -lcrypto -lws2_32 -lgdi32 -lcrypt32 \
			  -lz -lunistring -lidn -lintl -liconv
    cflags += -mthreads -DCURL_STATICLIB
    datafiles += cacert.pem
endef

define forDarwin
    datafiles += cacert.pem *.dylib
endef

lib.setup.sources = src/purest_json

# file for creating deken package
ifeq ($(findstring $(machine), x86_64 ia64), $(machine))
  deken.bits = 64
else
  deken.bits = 32
endif
ifeq ($(system), Windows)
  deken.ext = zip
  deken.pack = zip -9 -r
else
  deken.ext = tar.gz
  deken.pack = tar -zcvf
endif
deken.file = $(lib.name)-v$(lib.version)-($(system)-$(machine)-$(deken.bits))-externals
deken.tmp = deken-tmp
deken.folder = $(lib.name)

include pd-lib-builder/Makefile.pdlibbuilder

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
	  mv "$(deken.file).$(deken.ext)" ..;
